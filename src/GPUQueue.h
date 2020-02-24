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

        std::mutex                  m_inputLock;
        std::mutex                  m_inputOverloadLock;
        std::mutex                  m_outputLock;
        std::mutex                  m_predictionsTakenLock;
        std::mutex                  m_waitLock;

        std::atomic_int             m_inputsAdded;
        std::atomic_int             m_predictionsReferences;
        std::atomic_int             m_predictionsTaken;
        std::atomic_int             m_threadsWaiting;
        int                         m_batchHighestUnitCount;
        std::string                 m_states;
        
        std::vector<PyObject*>      m_networkOutput;
        PyObject *                  m_predictedValues;
        
        std::condition_variable     m_predictorThread;
        std::condition_variable     m_waitingThreads;
        std::condition_variable     m_waitTilPredictionsTaken;
        std::condition_variable     m_inputQueueFull;
        

    public:
        static GPUQueue& getInstance()
        {
            static GPUQueue instance;
            return instance;
        }

        int push_back(const std::pair<std::string, int> & str);
        void makePrediction();
        void wait();

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

