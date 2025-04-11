#include <functional>
#include <iostream>

class GenericGuard {
public:
    // Constructor: Accepts "begin" and "end" operations
    GenericGuard(std::function<void()> beginOp, std::function<void()> commitOp, std::function<void()> rollbackOp)
        : m_beginOp(beginOp), m_commitOp(commitOp), m_rollbackOp(rollbackOp), m_committed(false) {
        if (m_beginOp) {
            m_beginOp(); // Execute the "begin" operation
        }
    }

    // Commit the operation
    void commit() {
        if (!m_committed && m_commitOp) {
            m_commitOp(); // Execute the "commit" operation
            m_committed = true;
        }
    }

    // Rollback the operation
    void rollback() {
        if (!m_committed && m_rollbackOp) {
            m_rollbackOp(); // Execute the "rollback" operation
        }
    }

    // Destructor: Automatically rolls back if not committed
    ~GenericGuard() {
        if (!m_committed) {
            rollback();
        }
    }

private:
    std::function<void()> m_beginOp;    // Operation to begin the task
    std::function<void()> m_commitOp;   // Operation to commit the task
    std::function<void()> m_rollbackOp; // Operation to rollback the task
    bool m_committed;                   // Track whether the task was committed
};
