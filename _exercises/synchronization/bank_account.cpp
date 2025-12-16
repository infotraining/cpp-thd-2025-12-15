#include <iostream>
#include <thread>

namespace ver_1
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

        // void transfer(BankAccount& to, double amount)
        // {
        //     balance_ -= amount;
        //     to.balance_ += amount;
        // }

        void withdraw(double amount)
        {
            std::lock_guard lock(mtx_);
            balance_ -= amount;
        }

        void deposit(double amount)
        {
            std::lock_guard lock(mtx_);
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

inline namespace ver_2
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

    std::cout << "Balance: " << ba1.balance() << "\n";
    ba1.print();

    thd1.join();
    thd2.join();

    std::cout << "After all threads are done: ";
    ba1.print();
    ba2.print();
}
