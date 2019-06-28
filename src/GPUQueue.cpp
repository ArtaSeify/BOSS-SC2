#include "GPUQueue.h"
#include <chrono>

using namespace BOSS;

const size_t GPUQueue::MAX_SIZE = 64;
const int GPUQueue::TIMEOUT = 500;

GPUQueue::GPUQueue()
    : m_predictor()
    , m_gstate()
    , m_inputLock()
    , m_inputOverloadLock()
    , m_inputsAdded(0)
    , m_networkOutput()
    , m_predictedValues()
    , m_predictionsReferences(0)
    , m_outputLock()
    , m_predictorThread()
    , m_waitingThreads()
    , m_waitTilPredictionsTaken()
    , m_inputQueueFull()
    , m_predictionsTaken(0)
    , m_threadsWaiting(0)
{
    m_networkOutput.reserve(MAX_SIZE);
}

int GPUQueue::push_back(const std::string & str)
{
    std::unique_lock<std::mutex> ulOverload(m_inputOverloadLock);
    m_inputQueueFull.wait(ulOverload, [this] { return m_inputsAdded < MAX_SIZE; });

    std::scoped_lock<std::mutex> ul(m_inputLock);

    BOSS_ASSERT(m_inputsAdded < MAX_SIZE, "Trying to push into vector with size %i, but max size is %i", m_inputsAdded.load(), MAX_SIZE);

    int index = m_inputsAdded;
    ++m_inputsAdded;

    if (m_inputsAdded > 1)
    {
        m_states += "\n";
    }
    m_states += str;

    // queue is full, notify the thread that calls prediction
    if (m_inputsAdded == MAX_SIZE)
    {
        m_predictorThread.notify_one();
    }

    return index;
}

void GPUQueue::makePrediction()
{
    {
        std::scoped_lock<std::mutex> sl_output(m_outputLock);
        
        if (m_networkOutput.size() > 0)
        {
            while (m_predictionsReferences > 0);
            Py_DECREF(m_predictedValues);
            m_networkOutput.clear();
        }

        std::cout << "prediction size: " << m_inputsAdded << std::endl;

        m_predictedValues = PyObject_CallMethod(m_predictor, "predict", "(s)", m_states.c_str());
        BOSS_ASSERT(m_predictedValues != NULL, "No prediction result returned from Python code");

        for (int i = 0; i < m_inputsAdded; ++i)
        {
            m_networkOutput.push_back(PyList_GetItem(m_predictedValues, i));
        }

        m_predictionsTaken = 0;
    }
    m_waitingThreads.notify_all();
}

void GPUQueue::wait()
{
    // there are still predictions that need to be taken 
    // wait until all the predictions are taken
    {
        std::unique_lock<std::mutex> ulPredictions(m_predictionsTakenLock);
        while (!m_waitTilPredictionsTaken.wait_for(ulPredictions, std::chrono::microseconds(100), [this] { return m_predictionsTaken == m_networkOutput.size(); }));
    }

    std::unique_lock<std::mutex> waitul(m_waitLock);
    
    ++m_threadsWaiting;
    // first waiting thread waits for TIMEOUT time, the rest wait until woken up
    if (m_threadsWaiting == 1)
    {
        std::unique_lock<std::mutex> inputul(m_inputLock);
        waitul.unlock();

        // wait until the queue is full or timeout is reached
        m_predictorThread.wait_for(inputul, std::chrono::nanoseconds(TIMEOUT), [this] {return m_inputsAdded == MAX_SIZE; });
        
        {
            // wait until all the threads expecting a prediction have called the wait function
            while (m_threadsWaiting != m_inputsAdded);
            makePrediction();
        }

        // we can add to the input queue again
        m_inputsAdded = 0;
        m_states.clear();
        m_inputQueueFull.notify_all();
    }
    else
    {
        m_waitingThreads.wait(waitul, [this] { return m_predictionsTaken < m_networkOutput.size(); });
    }

    --m_threadsWaiting;
}

PyObject* GPUQueue::operator [] (int i)
{
    std::scoped_lock<std::mutex> sl_output(m_outputLock);

    BOSS_ASSERT(i < m_networkOutput.size(), "Trying to access an out of bounds index: %i, size of vector: %i", i, m_networkOutput.size());

    ++m_predictionsTaken;
    ++m_predictionsReferences;

    // we have taken all the predictions
    if (m_predictionsTaken == m_networkOutput.size())
    {
        m_waitTilPredictionsTaken.notify_all();
    }

    return m_networkOutput[i];
}