#include <iostream>
#include <mutex>
#include <thread>

inline namespace ver_1
{
    class BankAccount
    {
        const int id_;
        double balance_;
        mutable std::mutex mtx_;

    public:
        BankAccount(int id, double balance)
            : id_(id)
            , balance_(balance)
        {
        }

        void print() const
        {
            std::cout << "Bank Account #" << id_ << "; Balance = " << balance() << std::endl;
        }

        void transfer(BankAccount& to, double amount)
        {
            // if (this->id_ <  to.id_)
            // {
            //     std::lock_guard lk_from{mtx_};
            //     std::lock_guard lk_to{to.mtx_};

            //     balance_ -= amount;
            //     to.balance_ += amount;
            // }
            // else
            // {
            //     std::lock_guard lk_to{to.mtx_};
            //     std::lock_guard lk_from{mtx_};

            //     balance_ -= amount;
            //     to.balance_ += amount;
            // }

#if __cplusplus < 201703L
            std::unique_lock<std::mutex> lk_from{mtx_, std::defer_lock};
            std::unique_lock<std::mutex> lk_to{to.mtx_, std::defer_lock};
            std::lock(lk_from, lk_to); // protection from deadlock - begin of critical section
            balance_ -= amount;
            to.balance_ += amount;
#else
            std::scoped_lock lk{mtx_, to.mtx_};
            balance_ -= amount;
            to.balance_ += amount;
#endif
        } // end of critical section

        void withdraw(double amount)
        {
            std::unique_lock lk{mtx_, std::defer_lock};
            do_lock();
            balance_ -= amount;
        }

        void do_lock()
        {
            mtx_.lock();
        }

        void deposit(double amount)
        {
            std::scoped_lock lock(mtx_);
            balance_ += amount;
        }

        int id() const
        {
            return id_;
        }

        double balance() const
        {
            std::lock_guard lk{mtx_};
            return balance_;
        }
    };
} // namespace ver_1

namespace ver_2
{
    template <typename T>
    struct SynchronizedValue
    {
        T value;
        std::mutex mtx_value;

        [[nodiscard("Must be assigned to local variable to start critical section")]] std::lock_guard<std::mutex> lock()
        {
            return std::lock_guard{mtx_value};
        }
    };

    class BankAccount
    {
        const int id_;
        mutable SynchronizedValue<double> balance_;

    public:
        BankAccount(int id, double balance)
            : id_(id)
            , balance_(balance)
        {
        }

        void print() const
        {
            double current_balance{};
            {
                auto lk = balance_.lock();
                current_balance = balance_.value;
            }

            std::cout << "Bank Account #" << id_ << "; Balance = " << current_balance << std::endl;
        }

        // void transfer(BankAccount& to, double amount)
        // {
        //     balance_ -= amount;
        //     to.balance_ += amount;
        // }

        void withdraw(double amount)
        {
            auto lk = balance_.lock();
            balance_.value -= amount;
        }

        void deposit(double amount)
        {
            auto lk = balance_.lock();
            balance_.value += amount;
        }

        int id() const
        {
            return id_;
        }

        double balance() const
        {
            auto lk = balance_.lock();
            return balance_.value;
        }
    };
} // namespace ver_2

void make_withdraws(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.withdraw(1.0);
}

void make_deposits(BankAccount& ba, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        ba.deposit(1.0);
}

void make_transfers(BankAccount& from, BankAccount& to, int no_of_operations)
{
    for (int i = 0; i < no_of_operations; ++i)
        from.transfer(to, 1.0);
}

int main()
{
    const int NO_OF_ITERS = 10'000'000;

    BankAccount ba1(1, 10'000);
    BankAccount ba2(2, 10'000);

    std::cout << "Before threads are started: ";
    ba1.print();
    ba2.print();

    std::thread thd1(&make_withdraws, std::ref(ba1), NO_OF_ITERS);
    std::thread thd2(&make_deposits, std::ref(ba1), NO_OF_ITERS);
    std::thread thd3(&make_transfers, std::ref(ba1), std::ref(ba2), NO_OF_ITERS / 10); // ba1 -> ba2 : { ba1.lock(); | ba2.lock(); do_transfer(); }
    std::thread thd4(&make_transfers, std::ref(ba2), std::ref(ba1), NO_OF_ITERS / 10); // ba2 -> ba1 : { ba2.lock(); | ba1.lock(); do_transfer();}

    std::cout << "Balance: " << ba1.balance() << "\n";
    ba1.print();

    thd1.join();
    thd2.join();
    thd3.join();
    thd4.join();

    std::cout << "After all threads are done: ";
    ba1.print();
    ba2.print();
}
