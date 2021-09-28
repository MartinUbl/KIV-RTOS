#include <process/mutex.h>
#include <process/spinlock.h>
#include <process/process_manager.h>

#include <stdstring.h>

CMutex::CMutex()
    : IFile(NFile_Type_Major::Mutex), mWaiting_Processes{nullptr}
{
    spinlock_init(&mLock_State);
}

bool CMutex::Lock()
{
    auto* cur = sProcessMgr.Get_Current_Process();
    const unsigned int cpid = cur->pid;

    if (mHolder_PID == cpid)
        return false;

    while (spinlock_try_lock(&mLock_State) == Lock_Locked) // try_lock vraci puvodni hodnotu zamku - pokud vrati "zamceno", zamknout se jiste nepovedlo
    {
        // vlozime ho do fronty cekajicich
        TProcess_Queue_Node* nd = new TProcess_Queue_Node;
        nd->pid = cpid;
        nd->prev = nullptr;
        nd->next = mWaiting_Processes;
        mWaiting_Processes = nd;
        nd->next->prev = nd;

        // NOTE: tady by mohlo byt vhodne zavest podstav pro blokovany stav nad mutexem (a volat pak s parametrem)
        sProcessMgr.Block_Current_Process();
    }

    mHolder_PID = cpid;

    return true;
}

bool CMutex::Try_Lock()
{
    auto* cur = sProcessMgr.Get_Current_Process();
    const unsigned int cpid = cur->pid;

    if (mHolder_PID == cpid)
        return false;

    return spinlock_try_lock(&mLock_State);
}

bool CMutex::Unlock()
{
    auto* cur = sProcessMgr.Get_Current_Process();
    const unsigned int cpid = cur->pid;

    if (mHolder_PID != cpid || mLock_State != Lock_Locked)
        return false;

    mHolder_PID = 0;
    spinlock_unlock(&mLock_State);

    if (mWaiting_Processes)
    {
        // najdeme posledni proces v seznamu (nejdele cekajici)
        // samozrejme by slo O(1) pro takto jednoduchy priklad, ale pripravme se uz ted na inverzi priorit
        TProcess_Queue_Node* nd = mWaiting_Processes;
        while (nd->next != nullptr)
            nd = nd->next;

        // nd = zaznam procesu, ktery budeme probouzet a odebirat ho ze seznamu cekajicich

        // pokud to byl jediny cekajici, odebereme referenci i z instance mutexu
        if (mWaiting_Processes == nd)
            mWaiting_Processes = nullptr;

        // nechame proces prejit do stavu "runnable"
        sProcessMgr.Notify_Process(nd->pid);

        // nalinkujeme okolni uzly v seznamu, abychom mohli soucasny smazat
        if (nd->prev)
            nd->prev->next = nd->next;
        if (nd->next)
            nd->next->prev = nd->prev;

        delete nd;
    }

    return true;
}
