
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
class AdvancedScheduler : public SchedulingAlgorithm
{
public:
    /**
     * Returns the friendly name of the algorithm, for debugging and selection purposes.
     */
    const char *name() const override { return "adv"; }

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
     * Called every time a scheduling even t occurs, to cause the next eligible entity
     * to be chosen.  The next eligible entity might actually be the same entity, if
     * e.g. its timeslice has not expired.
     */
    SchedulingEntity *pick_next_entity() override
    {
        UniqueIRQLock l;

        // Pick from first considered runqueue if it is not empty
        runqueue_t &runqueue = multilevel_runqueue[order[index]];
        index = (index + 1) % num_order;
        if (!runqueue.empty())
        {
            SchedulingEntity *entity = runqueue.dequeue();
            runqueue.enqueue(entity);
            return entity;
        }

        // Otherwise continue to pick from highest priority to lowest priority
        for (runqueue_t &runqueue : multilevel_runqueue)
        {
            if (!runqueue.empty())
            {
                SchedulingEntity *entity = runqueue.dequeue();
                runqueue.enqueue(entity);
                return entity;
            }
        }

        // empty multilevel runqueue
        return nullptr;
    }

private:
    typedef SchedulingEntity entity_t;
    typedef List<entity_t *> runqueue_t;

    // From highest priority (0) to lowest priority (num_levels - 1)
    static constexpr unsigned int num_levels = 4;
    runqueue_t multilevel_runqueue[num_levels];

    // First considered runqueue
    static constexpr unsigned int num_order = 15;
    static constexpr unsigned int order[num_order] = {0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
    int index = 0; // initialise

    // Obtain the corresponding runqueue of the given entity
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
            syslog.messagef(LogLevel::FATAL, "Unknown prioity for entity %s.", entity.name().c_str());
            arch_abort();
        }
    }
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

RegisterScheduler(AdvancedScheduler);