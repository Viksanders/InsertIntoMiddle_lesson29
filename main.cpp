#include <iostream>
#include <thread>
#include <chrono>
#include <functional>
#include <mutex>
#include <future>
#include <string>

std::mutex m_cout;

void locked_output(const std::string& msg)
{
    m_cout.lock();
    std::cout << msg;
    m_cout.unlock();
}

struct Node
{
    int value;
    Node* next;
    std::mutex* node_mutex;
};

class FineGrainedQueue
{
public:
    //реализованы только некоторые вспомогательные методы! класс не полный!!!
    FineGrainedQueue();
    void pushback(int val);                         //просто метод для более удобного тестирования
    void print();                                   //просто метод для более удобного тестирования
    void insertIntoMiddle(int val, int pos);        //!!!ИТОГОВОЕ ЗАДАНИЕ ПО МОДУЛЮ 29!!!
private:
    Node* head;
    std::mutex* queue_mutex;
};

FineGrainedQueue::FineGrainedQueue()
{
    srand(1252131);
    head = NULL;
    queue_mutex = new std::mutex;
}

void FineGrainedQueue::pushback(int val)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + rand() % 50));
    std::lock_guard<std::mutex> m(*queue_mutex);
    if (head == NULL)
    {
        head = new Node;
        head->next = NULL;
        head->value = val;
        head->node_mutex = new std::mutex;
        return;
    }
    else
    {
        Node* loopPtr = head;
        while (loopPtr->next != NULL)
        {
            loopPtr = loopPtr->next;
        }
        Node* newNode = new Node;
        newNode->next = NULL;
        newNode->value = val;
        newNode->node_mutex = new std::mutex;
        loopPtr->next = newNode;
        return;
    }
}

void FineGrainedQueue::print()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + rand() % 50));
    std::lock_guard<std::mutex> m(*queue_mutex);
    Node* loopPtr = head;
    while (loopPtr != NULL)
    {
        locked_output(std::to_string(loopPtr->value));
        locked_output(" ");
        loopPtr = loopPtr->next;
    }
    locked_output("\n");
}

void FineGrainedQueue::insertIntoMiddle(int val, int pos)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50 + rand() % 50));

    //по заданию список не пуст, вставка происходит не в начало
    int counter = 0;    //счетчик для вставки в нужную позицию, при этом нулевая позиция - это следующий за гловным элемент
                        //т.к. по условию перед головным элементом не ставим
                        //т.е. если оставить pos = 0, то вставка будет следом за первым элементом. Если pos > size, то вставка осуществляется в самый конец

    Node* prev;

    queue_mutex->lock();        //лочим весь список, т.к. head может измениться на лету (например, быть удалён)

    Node* loopPtr = head;

    loopPtr->node_mutex->lock();        //здесь сразу же лочится рассматриваемая нода, к которой, возможно, будет прицеплена новая нода
                                        //т.к. список односвязный, то достаточно залочить только один узел, который будет подвергнут модификациям
    queue_mutex->unlock();


    while ((loopPtr->next != NULL) && (counter != pos))
    {
        ++counter;
        prev = loopPtr;                 //запомним предыдущую ноду
        loopPtr = loopPtr->next;
        loopPtr->node_mutex->lock();    //лочим текущую ноду, которая может оказаться целевой или последней нодой списка, т.к. именно она будет подвергнута модификациям
        prev->node_mutex->unlock();     //предыдущую пройденную ноду уже можно анлочить
    }


    Node* ToInsert = new Node;
    ToInsert->value = val;
    ToInsert->node_mutex = new std::mutex;
    ToInsert->node_mutex->lock();       //решил тоже залочить вновь создаваемую ноду
                                        //чтобы пока она "подключается" к родительской ноде, не было вклинивания из другого потока
                                        //родительская нода так же должна быть залочена

    ToInsert->next = loopPtr->next;     //соединяем созданную ноду с правой половиной списка
    loopPtr->next = ToInsert;           //цепляем левую половину списка к созданной ноде

    //теперь можно всё разлочить
    loopPtr->node_mutex->unlock();      //эта нода осталась залоченной ещё с цикла    while ( (loopPtr->next != NULL) && (counter != pos) )       (см выше) её анлоичим первой
    ToInsert->node_mutex->unlock();     //созданная нода была залочена сразу после создания ещё до установления связей с правой и левой частями списка, эту ноду анлочим в самом конце
}

int main()
{
    FineGrainedQueue FGQ;

    /*  //Single thread works fine
    FGQ.pushback(1);
    FGQ.pushback(2);
    FGQ.pushback(3);
    FGQ.pushback(4);

    std::cout << "FULL DATA: \n";
    FGQ.print();

    FGQ.insertIntoMiddle(222, 0);

    std::cout << "FULL DATA 2: \n";
    FGQ.print();
    //*/


    //* //Multithread test
    std::future<void> f1 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(1); });
    std::future<void> f2 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(2); });
    std::future<void> f3 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(3); });

    std::future<void> f7 = std::async(std::launch::async, [&FGQ]() { FGQ.print(); });

    std::future<void> f4 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(4); });
    std::future<void> f5 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(5); });
    std::future<void> f6 = std::async(std::launch::async, [&FGQ]() { FGQ.pushback(6); });

    std::future<void> f8 = std::async(std::launch::async, [&FGQ]() { FGQ.print(); });

    f1.wait();
    f2.wait();
    f3.wait();
    f4.wait();
    f5.wait();
    f6.wait();

    f7.wait();

    f8.wait();

    std::cout << "FULL DATA: \n";
    FGQ.print();

    std::cout << "\n\n\n";

    std::future<void> f9 = std::async(std::launch::async, [&FGQ]() { FGQ.insertIntoMiddle(10, 1); });
    std::future<void> f10 = std::async(std::launch::async, [&FGQ]() { FGQ.insertIntoMiddle(11, 1); });
    std::future<void> f11 = std::async(std::launch::async, [&FGQ]() { FGQ.insertIntoMiddle(12, 1); });

    std::future<void> f14 = std::async(std::launch::async, [&FGQ]() { FGQ.print(); });

    std::future<void> f12 = std::async(std::launch::async, [&FGQ]() { FGQ.insertIntoMiddle(13, 1); });
    std::future<void> f13 = std::async(std::launch::async, [&FGQ]() { FGQ.insertIntoMiddle(14, 222); });

    std::future<void> f15 = std::async(std::launch::async, [&FGQ]() { FGQ.print(); });


    f9.wait();
    f10.wait();
    f11.wait();
    f12.wait();
    f13.wait();
    f14.wait();
    f15.wait();

    std::cout << "FULL DATA 2: \n";
    FGQ.print();

    //*/

    return 0;
}
