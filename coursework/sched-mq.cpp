/*
 * The Priority Task Scheduler
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 1
 */

#include <infos/kernel/sched.h>
#include <infos/kernel/thread.h>
#include <infos/kernel/log.h>
#include <infos/util/list.h>
#include <infos/util/lock.h>

using namespace infos::kernel;
using namespace infos::util;

/**
 * A Multiple Queue priority scheduling algorithm
 */
class MultipleQueuePriorityScheduler : public SchedulingAlgorithm
{
public:
    /**
     * Returns the friendly name of the algorithm, for debugging and selection purposes.
     */
    const char *name() const override { return "mq"; }

    /**
     * Called during scheduler initialisation.
     */
    void init()
    {
    }

    /**
     * Called when a scheduling entity becomes eligible for running.
     * @param entity
     */
    void add_to_runqueue(SchedulingEntity &entity) override
    {
        UniqueIRQLock l;
        get_runqueue(entity)->enqueue(&entity);
    }

    /**
     * Called when a scheduling entity is no longer eligible for running.
     * @param entity
     */
    void remove_from_runqueue(SchedulingEntity &entity) override
    {
        UniqueIRQLock l;
        get_runqueue(entity)->remove(&entity);
    }

    /**
     * Called every time a scheduling event occurs, to cause the next eligible entity
     * to be chosen.  The next eligible entity might actually be the same entity, if
     * e.g. its timeslice has not expired.
     */
    SchedulingEntity *pick_next_entity() override
    {
        UniqueIRQLock l;
        // Loop from highest priority (0) to lowest priority (num_levels - 1)
        for (runqueue_t &runqueue : multilevel_runqueue)
        {
            if (!runqueue.empty())
            {
                SchedulingEntity *entity = runqueue.dequeue();
                runqueue.enqueue(entity);
                return entity;
            }
        }
        // Empty multilevel runqueue
        return nullptr;
    }

private:
    typedef SchedulingEntity entity_t;
    typedef List<entity_t *> runqueue_t;

    // From highest priority (0) to lowest priority (num_levels - 1)
    static constexpr unsigned int num_levels = 4;
    runqueue_t multilevel_runqueue[num_levels];

    /** Obtain the corresponding runqueue of the given entity.
     * @param entity the scheduling entity
     * @return Returns the corresponding runqueue of the scheduling entity
     */
    runqueue_t *get_runqueue(entity_t &entity)
    {
        switch (entity.priority())
        {
        case SchedulingEntityPriority::REALTIME:
            return &multilevel_runqueue[0];
        case SchedulingEntityPriority::INTERACTIVE:
            return &multilevel_runqueue[1];
        case SchedulingEntityPriority::NORMAL:
            return &multilevel_runqueue[2];
        case SchedulingEntityPriority::DAEMON:
            return &multilevel_runqueue[3];
        default:
            // info-usr/inc/infos.h SchedulingEntityPriority has IDLE which is not defined in kernel
            // need to handle unexpected priority, just put it in lowest priority level
            syslog.messagef(LogLevel::FATAL, "Unknown prioity for entity %s.", entity.name().c_str());
            return &multilevel_runqueue[3];
        }
    }
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(MultipleQueuePriorityScheduler);