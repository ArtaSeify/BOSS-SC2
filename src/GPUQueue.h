#pragma once
#include "Common.h"

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace BOSS
{
    class GPUQueue
    {
        const static int TIMEOUT;
        const static size_t MAX_SIZE;

        PyObject*                   m_predictor;            // reference to module that contains the network used for evaluations
        PyGILState_STATE            m_gstate;

        std::vector<std::string>    m_networkInput;
        std::mutex                  m_inputLock;
        std::mutex                  m_inputOverloadLock;
        std::atomic_int             m_inputsAdded;
        
        std::vector<PyObject*>      m_networkOutput;
        PyObject *                  m_predictedValues;
        std::atomic_int             m_predictionsReferences;
        std::mutex                  m_outputLock;
        std::mutex                  m_predictionsTakenLock;

        std::condition_variable     m_predictorThread;
        std::condition_variable     m_waitingThreads;
        std::condition_variable     m_waitTilPredictionsTaken;
        std::condition_variable     m_inputQueueFull;
        bool                        m_predictionsDone;
        std::atomic_int             m_predictionsTaken;
        std::atomic_int             m_threadsWaiting;

    public:
        static GPUQueue& getInstance()
        {
            static GPUQueue instance;
            return instance;
        }

        int push_back(const std::string& str);
        void makePrediction();
        void wait();
        bool predictionsDone() const { return m_predictionsDone; }

        void setPredictorFunction(PyObject* predictor) { m_predictor = predictor; }
        void getPythonInterpretor() { m_gstate = PyGILState_Ensure(); }
        void releasePythonInterpretor() { PyGILState_Release(m_gstate); }

        PyObject* operator [] (int i);

        void decPredictionReference() { --m_predictionsReferences; }

        GPUQueue(GPUQueue const&) = delete;
        void operator=(GPUQueue const&) = delete;

    private:
        GPUQueue();
    };
}

