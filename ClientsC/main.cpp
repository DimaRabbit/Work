#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <pqxx/pqxx>

class Customer {
public:
    std::string firstName;
    std::string lastName;
    std::string email;
    std::vector<std::string> phones;

    // Конструктор по умолчанию
    Customer() {}

    // Конструктор с параметрами
    Customer(const std::string& fName, const std::string& lName, const std::string& mail)
        : firstName(fName), lastName(lName), email(mail) {}
};

class CustomerManager {
private:
    pqxx::connection& conn;  // Ссылка на подключение к базе данных

public:
    // Конструктор класса
    CustomerManager(pqxx::connection& db_conn) : conn(db_conn) {}

    // Метод создания таблицы клиентов
    void createCustomerTable() {
        pqxx::work txn(conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS customers (
                id SERIAL PRIMARY KEY,
                first_name VARCHAR(50),
                last_name VARCHAR(50),
                email VARCHAR(100) UNIQUE
            );
        )");
        txn.commit();
        std::cout << "Customer table created successfully." << std::endl;
    }

    // Метод создания таблицы телефонов
    void createPhoneTable() {
        pqxx::work txn(conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS phones (
                id SERIAL PRIMARY KEY,
                customer_id INT REFERENCES customers(id) ON DELETE CASCADE,
                phone_number VARCHAR(15)
            );
        )");
        txn.commit();
        std::cout << "Phone table created successfully." << std::endl;
    }

    // Метод добавления нового клиента
    int addCustomer(const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(conn);
        pqxx::result result = txn.exec_params(R"(
            INSERT INTO customers (first_name, last_name, email)
            VALUES ($1, $2, $3) RETURNING id;
        )", firstName, lastName, email);

        int id = result[0][0].as<int>();
        txn.commit();
        std::cout << "Customer added with ID: " << id << std::endl;
        return id;
    }

    // Добавление телефона клиенту
    void addPhone(int customerId, const std::string& phone) {
        pqxx::work txn(conn);
        txn.exec_params(R"(
            INSERT INTO phones (customer_id, phone_number)
            VALUES ($1, $2);
        )", customerId, phone);
        txn.commit();
        std::cout << "Phone added for customer ID: " << customerId << std::endl;
    }

    // Обновление данных клиента
    void updateCustomer(int customerId, const std::string& firstName, const std::string& lastName, const std::string& email) {
        pqxx::work txn(conn);
        txn.exec_params(R"(
            UPDATE customers SET first_name = $1, last_name = $2, email = $3 WHERE id = $4;
        )", firstName, lastName, email, customerId);
        txn.commit();
        std::cout << "Customer with ID " << customerId << " updated." << std::endl;
    }

    // Удаление телефона клиента
    void removePhone(int customerId, const std::string& phone) {
        pqxx::work txn(conn);
        txn.exec_params(R"(
            DELETE FROM phones WHERE customer_id = $1 AND phone_number = $2;
        )", customerId, phone);
        txn.commit();
        std::cout << "Phone removed for customer ID: " << customerId << std::endl;
    }

    // Удаление клиента по ID
    void removeCustomer(int customerId) {
        pqxx::work txn(conn);
        txn.exec_params(R"(
            DELETE FROM customers WHERE id = $1;
        )", customerId);
        txn.commit();
        std::cout << "Customer with ID " << customerId << " removed." << std::endl;
    }

    // Поиск клиента по имени, фамилии, email или телефону
    void findCustomer(const std::string& searchTerm) {
        pqxx::work txn(conn);
        pqxx::result result = txn.exec_params(R"(
            SELECT c.id, c.first_name, c.last_name, c.email, p.phone_number
            FROM customers c
            LEFT JOIN phones p ON c.id = p.customer_id
            WHERE c.first_name = $1 OR c.last_name = $1 OR c.email = $1 OR p.phone_number = $1;
        )", searchTerm);

        if (result.empty()) {
            std::cerr << "Error: Customer not found." << std::endl;
        }
        else {
            for (const auto& row : result) {
                std::cout << "Customer found: " << row["first_name"].as<std::string>() << " "
                    << row["last_name"].as<std::string>() << ", Email: " << row["email"].as<std::string>() << std::endl;
                if (!row["phone_number"].is_null()) {
                    std::cout << "Phone: " << row["phone_number"].as<std::string>() << std::endl;
                }
            }
        }
    }

    // Вывод всех клиентов
    void listCustomers() const {
        pqxx::work txn(conn);
        pqxx::result result = txn.exec(R"(
            SELECT c.id, c.first_name, c.last_name, c.email, p.phone_number
            FROM customers c
            LEFT JOIN phones p ON c.id = p.customer_id
            ORDER BY c.id;
        )");

        for (const auto& row : result) {
            std::cout << "ID: " << row["id"].as<int>() << ", Name: " << row["first_name"].as<std::string>()
                << " " << row["last_name"].as<std::string>() << ", Email: " << row["email"].as<std::string>() << std::endl;
            if (!row["phone_number"].is_null()) {
                std::cout << "Phone: " << row["phone_number"].as<std::string>() << std::endl;
            }
        }
    }
};

int main() {
    try {
        // Строка подключения к базе данных PostgreSQL
        std::string connectionString =
            "host=localhost port=5432 dbname=postgres user=postgres password=extreme190287";

        // Подключение к базе данных
        pqxx::connection conn(connectionString);

        // Создание объекта CustomerManager для работы с клиентами
        CustomerManager manager(conn);

        // Создание таблиц в базе данных
        manager.createCustomerTable();
        manager.createPhoneTable();

        // Добавляем клиентов
        int id1 = manager.addCustomer("John", "Doe", "john.doe@example.com");
        int id2 = manager.addCustomer("Jane", "Smith", "jane.smith@example.com");

        // Добавляем телефоны
        manager.addPhone(id1, "+1234567890");
        manager.addPhone(id1, "+0987654321");
        manager.addPhone(id2, "+1122334455");

        // Обновляем данные клиента
        manager.updateCustomer(id1, "John", "Doe", "john.doe@newdomain.com");

        // Удаляем телефон
        manager.removePhone(id1, "+0987654321");

        // Ищем клиента по email
        manager.findCustomer("jane.smith@example.com");

        // Удаляем клиента
        manager.removeCustomer(id2);

        // Вывод всех клиентов
        manager.listCustomers();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}