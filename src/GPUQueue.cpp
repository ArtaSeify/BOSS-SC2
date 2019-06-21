#include "GPUQueue.h"
#include <chrono>

using namespace BOSS;

const size_t GPUQueue::MAX_SIZE = 24;
const int GPUQueue::TIMEOUT = 1;

GPUQueue::GPUQueue()
    : m_predictor()
    , m_gstate()
    , m_networkInput()
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
    , m_predictionsDone(false)
    , m_predictionsTaken(0)
    , m_threadsWaiting(0)
{
    m_networkInput.reserve(MAX_SIZE);
}

int GPUQueue::push_back(const std::string & str)
{
    //std::cout << "push_back called" << std::endl;
    std::scoped_lock<std::mutex> lgOverload(m_inputOverloadLock);

    // wait until we have space in queue
    while (m_inputsAdded == MAX_SIZE);
    
    //std::cout << "push_back called" << std::endl;
    std::scoped_lock<std::mutex> lg(m_inputLock);

    BOSS_ASSERT(m_networkInput.size() < MAX_SIZE, "Trying to push into vector with size %i, but max size is %i", m_networkInput.size(), MAX_SIZE);

    ++m_inputsAdded;
    m_networkInput.push_back(str);
    
    int index = int(m_inputsAdded - 1);
    //std::cout << "elements added to queue: " << m_inputsAdded << std::endl;

    // queue is full, notify the thread that calls prediction
    if (m_inputsAdded == MAX_SIZE)
    {
        m_predictorThread.notify_one();
    }
    return index;
}

void GPUQueue::makePrediction()
{
    //std::cout << "makePrediction called" << std::endl;
    // wait until all the predictions are taken
    while (m_predictionsTaken != m_networkOutput.size());
    //std::cout << "predictions taken match" << std::endl;
    // wait until all the threads expecting a prediction have called the wait function
    //std::cout << "waiting: " << m_threadsWaiting << std::endl;
    //std::cout << "network input size: " << m_networkInput.size() << std::endl;
    while (m_threadsWaiting != m_inputsAdded);
    //std::cout << "threads waiting match" << std::endl;

    {
        std::scoped_lock<std::mutex> lg_input(m_inputLock);
        //std::cout << "took input lock" << std::endl;

        std::scoped_lock<std::mutex> lg_output(m_outputLock);
        //std::cout << "took output lock" << std::endl;
        
        if (m_networkOutput.size() > 0)
        {
            //std::cout << m_predictionsReferences << std::endl;
            while (m_predictionsReferences > 0);
            Py_DECREF(m_predictedValues);
            m_networkOutput.clear();
        }

        std::string states = "";
        for (int i = 0; i < m_inputsAdded; ++i)
        {
            states += m_networkInput[i];
            if (i != m_inputsAdded - 1)
            {
                states += "\n";
            }
        }

        //std::cout << "prediction size: " << m_inputsAdded << std::endl;
        m_predictedValues = PyObject_CallMethod(m_predictor, "predict", "(s)", states.c_str());
        BOSS_ASSERT(m_predictedValues != NULL, "No prediction result returned from Python code");

        for (int i = 0; i < m_inputsAdded; ++i)
        {
            m_networkOutput.push_back(PyList_GetItem(m_predictedValues, i));
            //std::cout << "added to output queue" << std::endl;
        }
        //std::cout << "predictions added" << std::endl;

        // we can add to the input queue again
        m_networkInput.clear();
        m_inputsAdded = 0;

        m_predictionsTaken = 0;
        m_predictionsDone = true;
    }
    m_waitingThreads.notify_all();
    //std::cout << "predictions done" << std::endl;
}

void GPUQueue::wait()
{
    while (m_predictionsTaken != m_networkOutput.size());

    //std::cout << "wait called" << std::endl;
    std::unique_lock<std::mutex> ul(m_outputLock);    
    //std::cout << "waiting for predictions to be taken" << std::endl;
    // wait until all the predictions are taken
    
    //m_waitTilPredictionsTaken.wait(ul, [this] { return m_predictionsTaken == m_networkOutput.size(); });
    //std::cout << "finished waiting for predictions to be taken" << std::endl;

    ++m_threadsWaiting;
    //std::cout << "added to threads waiting. There are now " << m_threadsWaiting << std::endl;
    // first waiting thread waits for TIMEOUT time, the rest wait until woken up
    if (m_threadsWaiting == 1)
    {
        //std::cout << "first waiting thread" << std::endl;
        m_predictorThread.wait_for(ul, std::chrono::milliseconds(TIMEOUT), [this] {return m_inputsAdded == MAX_SIZE; });
        {
            //std::cout << "Calling prediction on " << m_networkInput.size() << " elements" << std::endl;
            ul.unlock();
            makePrediction();
            ul.lock();
        }
        //std::cout << "woke up first thread" << std::endl;
    }
    else
    {
        m_waitingThreads.wait(ul, [this] { return m_predictionsDone; });
        //std::cout << "WOKE UP OTHER THREAD WOKE UP OTHER THREAD WOKE UP OTHER THREAD" << std::endl;
    }
    
    --m_threadsWaiting;
    //std::cout << "removed from threads waiting. There are now " << m_threadsWaiting << std::endl;
}

PyObject* GPUQueue::operator [] (int i)
{
    //std::cout << "operator [] called" << std::endl;
    std::scoped_lock<std::mutex> lg_input(m_outputLock);

    BOSS_ASSERT(i < m_networkOutput.size(), "Trying to access an out of bounds index %i", i);
    ++m_predictionsTaken;
    ++m_predictionsReferences;
    
    // we have taken all the predictions
    if (m_predictionsTaken == m_networkOutput.size())
    {
        m_predictionsDone = false;
    }
    //std::cout << "predictions taken from queue: " << m_predictionsTaken << std::endl;
    return m_networkOutput[i];
}