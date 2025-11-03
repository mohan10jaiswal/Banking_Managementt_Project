#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <memory>
#include <vector>
#include <iomanip>
#include <conio.h>
#include <functional> // For hashing
using namespace std;

// ================== Utility Functions ==================
string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Hash password
string hashPassword(const string &password) {
    size_t hashed = hash<string>{}(password);
    return to_string(hashed);
}

// Input password with masking
string inputPassword() {
    string password;
    char ch;
    while (true) {
        ch = _getch();
        if (ch == 13) break;        // Enter
        else if (ch == 8) {         // Backspace
            if (!password.empty()) {
                cout << "\b \b";
                password.pop_back();
            }
        } else {
            password.push_back(ch);
            cout << '*';
        }
    }
    cout << endl;
    return password;
}

// Generate unique account number
int generateAccountNumber() {
    ifstream fin("accounts.txt");
    string line;
    int maxAcc = 100000; // starting account number
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string accNoStr;
        getline(ss, accNoStr, ',');
        int accNo = stoi(accNoStr);
        if (accNo > maxAcc) maxAcc = accNo;
    }
    fin.close();
    return maxAcc + 1;
}

// Check if phone number exists
bool phoneExists(const string &phone) {
    ifstream fin("accounts.txt");
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string accNo, name, pass, type, bal, ph;
        getline(ss, accNo, ','); getline(ss, name, ','); getline(ss, pass, ',');
        getline(ss, type, ','); getline(ss, bal, ','); getline(ss, ph, ',');
        if (ph == phone) return true;
    }
    fin.close();
    return false;
}

// ================== Base Class ==================
class Account {
protected:
    int accNo;
    string name, password, phone;
    double balance;

public:
    Account() : accNo(0), balance(0.0) {}
    virtual ~Account() {}

    void setAccNo(int no) { accNo = no; }
    int getAccNo() const { return accNo; }

    void setName(const string &n) { name = n; }
    string getName() const { return name; }

    void setPassword(const string &p) { password = p; }
    string getPassword() const { return password; }

    void setPhone(const string &p) { phone = trim(p); }
    string getPhone() const { return phone; }

    void setBalance(double b) { balance = b; }
    double getBalance() const { return balance; }

    virtual void deposit(double amt) {
        balance += amt;
        saveTransaction("Deposit", amt);
    }

    virtual bool withdraw(double amt) {
        if (balance >= amt) {
            balance -= amt;
            saveTransaction("Withdraw", amt);
            return true;
        }
        return false;
    }

    void saveTransaction(const string &type, double amt) {
        ofstream fout("transactions_" + to_string(accNo) + ".txt", ios::app);
        time_t now = time(0);
        char *dt = ctime(&now);
        dt[strlen(dt)-1] = '\0';
        fout << dt << "," << type << "," << amt << "," << balance << endl;
        fout.close();
    }

    void showTransactions() {
        ifstream fin("transactions_" + to_string(accNo) + ".txt");
        string line;
        cout << "\n--- Transaction History ---\n";
        while (getline(fin, line)) cout << line << endl;
        fin.close();
    }

    void saveAccount(const string &accType) {
        ofstream fout("accounts.txt", ios::app);
        fout << accNo << "," << name << "," << password << "," 
             << accType << "," << balance << "," << phone << endl;
        fout.close();
    }

    void updateAccount(const string &accType) {
        ifstream fin("accounts.txt");
        ofstream fout("temp.txt");
        string line;
        while (getline(fin, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string accNoStr;
            getline(ss, accNoStr, ',');
            if (stoi(accNoStr) == accNo) {
                fout << accNo << "," << name << "," << password << "," 
                     << accType << "," << balance << "," << phone << endl;
            } else fout << line << endl;
        }
        fin.close(); fout.close();
        remove("accounts.txt");
        rename("temp.txt", "accounts.txt");
    }

    static unique_ptr<Account> fromRecordLine(const string &line);
};

// ================== Derived Classes ==================
class SavingsAccount : public Account {
public:
    bool withdraw(double amt) override {
        if (getBalance() - amt >= 1000) return Account::withdraw(amt);
        cout << "Cannot withdraw. Minimum balance 1000 required.\n";
        return false;
    }
};

class CurrentAccount : public Account {
public:
    bool withdraw(double amt) override {
        if (getBalance() - amt >= -5000) return Account::withdraw(amt);
        cout << "Overdraft limit exceeded (-5000)\n";
        return false;
    }
};

// ================== Factory ==================
unique_ptr<Account> Account::fromRecordLine(const string &line) {
    stringstream ss(line);
    string accNoStr, n, p, t, balStr, ph;
    getline(ss, accNoStr, ','); getline(ss, n, ','); getline(ss, p, ',');
    getline(ss, t, ','); getline(ss, balStr, ','); getline(ss, ph, ',');
    unique_ptr<Account> acc;
    if (t=="Savings") acc.reset(new SavingsAccount());
    else acc.reset(new CurrentAccount());
    acc->setAccNo(stoi(accNoStr));
    acc->setName(n); acc->setPassword(p); acc->setBalance(stod(balStr));
    acc->setPhone(ph);
    return acc;
}

// ================== User Class ==================
class User {
    unique_ptr<Account> accPtr;
    string accType;

    bool canDoTransaction() {
        string filename = "daily_txn_" + to_string(accPtr->getAccNo()) + ".txt";
        string today;
        time_t now = time(0); 
        tm *ltm = localtime(&now);
        today = to_string(1900+ltm->tm_year) + "-" + 
                to_string(1+ltm->tm_mon) + "-" + 
                to_string(ltm->tm_mday);

        int count = 0;
        string line, fileDate;
        ifstream fin(filename);
        if(fin.is_open() && getline(fin,line)) {
            stringstream ss(line);
            getline(ss,fileDate,','); ss >> count;
            if(fileDate != today) count = 0; // reset if new day
        }
        fin.close();

        if(count >= 5) return false;

        // Update count
        ofstream fout(filename, ios::trunc);
        fout << today << "," << (count+1);
        fout.close();
        return true;
    }

public:
    User(unique_ptr<Account> a, string type) : accPtr(move(a)), accType(type) {}

    void userMenu() {
        int choice;
        do {
            cout << "\n--- Account Menu ---\n";
            cout << "1. Check Balance\n2. Deposit\n3. Withdraw\n4. Transactions\n5. Request Account Deletion\n6. Logout\n";
            cout << "Enter choice: "; cin >> choice; cin.ignore();
            if (choice==1) cout << "Balance: " << accPtr->getBalance() << endl;
            else if (choice==2) {
                if(!canDoTransaction()) { cout << "Daily transaction limit reached (5 per day)\n"; continue; }
                double amt; cout << "Enter deposit amount: "; cin >> amt; cin.ignore();
                accPtr->deposit(amt); accPtr->updateAccount(accType); cout << "Deposit successful.\n";
            } else if (choice==3) {
                if(!canDoTransaction()) { cout << "Daily transaction limit reached (5 per day)\n"; continue; }
                double amt; cout << "Enter withdrawal amount: "; cin >> amt; cin.ignore();
                if(accPtr->withdraw(amt)) accPtr->updateAccount(accType);
            } else if (choice==4) accPtr->showTransactions();
            else if (choice==5) {
                ofstream fout("deletion_requests.txt", ios::app);
                fout << accPtr->getAccNo() << "," << accPtr->getName() << "," << accPtr->getPhone() << endl;
                fout.close();
                cout << "Account deletion request submitted. Admin will review it.\n";
            }
        } while(choice!=6);
    }
};

// ================== Admin Class ==================
class Admin {
public:
    void adminMenu() {
        int choice;
        do {
            cout << "\n--- Admin Menu ---\n";
            cout << "1. List All Users\n2. Search User\n3. Approve Deletion Requests\n4. Logout\n";
            cout << "Enter choice: "; cin >> choice; cin.ignore();
            if(choice==1) listAllAccounts();
            else if(choice==2) searchAccount();
            else if(choice==3) approveDeletionRequests();
        } while(choice!=4);
    }

    void listAllAccounts() {
        ifstream fin("accounts.txt"); string line;
        cout << "\n" 
             << left << setw(10) << "AccNo" 
             << setw(20) << "Name" 
             << setw(10) << "Type" 
             << setw(10) << "Balance" 
             << setw(15) << "Phone" << "\n";

        while(getline(fin,line)) {
            if(line.empty()) continue;
            stringstream ss(line); string accNo,name,pass,type,balance,phone;
            getline(ss,accNo,','); getline(ss,name,','); getline(ss,pass,',');
            getline(ss,type,','); getline(ss,balance,','); getline(ss,phone,',');
            cout << left << setw(10) << accNo 
                 << setw(20) << name 
                 << setw(10) << type 
                 << setw(10) << balance 
                 << setw(15) << phone << "\n";
        }
        fin.close();
    }

    void searchAccount() {
        int accNo; cout << "Enter Account No: "; cin >> accNo; cin.ignore();
        ifstream fin("accounts.txt"); string line; bool found=false;
        while(getline(fin,line)) {
            if(line.empty()) continue;
            auto acc=Account::fromRecordLine(line);
            if(acc->getAccNo()==accNo) {
                cout << "AccNo: " << acc->getAccNo() << "\nName: " << acc->getName() 
                     << "\nBalance: " << acc->getBalance() << "\nPhone: " << acc->getPhone() << "\n";
                acc->showTransactions(); found=true; break;
            }
        }
        if(!found) cout << "Account not found!\n";
        fin.close();
    }

    void approveDeletionRequests() {
        ifstream fin("deletion_requests.txt"); string line;
        vector<string> pendingRequests;
        while(getline(fin,line)) if(!line.empty()) pendingRequests.push_back(line);
        fin.close();

        for(auto &req: pendingRequests) {
            stringstream ss(req); string accNoStr,name,phone;
            getline(ss,accNoStr,','); getline(ss,name,','); getline(ss,phone,',');
            int accNo=stoi(accNoStr);
            char ch; cout << "Approve deletion for " << name << " (AccNo " << accNo << ")? (y/n): ";
            cin >> ch; cin.ignore();
            if(ch=='y' || ch=='Y') {
                // Delete from accounts.txt
                ifstream fin2("accounts.txt"); ofstream fout("temp.txt"); string line2;
                while(getline(fin2,line2)) {
                    if(line2.empty()) continue;
                    stringstream ss2(line2); string accNoStr2; getline(ss2,accNoStr2,',');
                    if(stoi(accNoStr2)!=accNo) fout << line2 << endl;
                }
                fin2.close(); fout.close();
                remove("accounts.txt"); rename("temp.txt","accounts.txt");
                remove(("transactions_" + to_string(accNo) + ".txt").c_str());
                remove(("daily_txn_" + to_string(accNo) + ".txt").c_str());
                cout << "Deleted account " << accNo << "\n";
            }
        }
        // Clear deletion requests
        ofstream fout("deletion_requests.txt", ios::trunc); fout.close();
    }
};

// ================== Helper Functions ==================
unique_ptr<Account> findAccountByNo(int accNo, string &accType) {
    ifstream fin("accounts.txt"); string line;
    while(getline(fin,line)) {
        if(line.empty()) continue;
        auto acc=Account::fromRecordLine(line);
        accType=(dynamic_cast<SavingsAccount*>(acc.get()))?"Savings":"Current";
        if(acc->getAccNo()==accNo) return acc;
    }
    return nullptr;
}

int generateOTP() { return rand()%900000 + 100000; }

void resetPasswordByOTP() {
    string phone; cout << "Enter your registered phone: "; getline(cin,phone); phone=trim(phone);
    if(!phoneExists(phone)) { cout << "Phone not found!\n"; return; }
    int otp = generateOTP(); cout << "OTP (for demo): " << otp << endl;
    int entered; cout << "Enter OTP: "; cin >> entered; cin.ignore();
    if(entered!=otp) { cout << "Wrong OTP!\n"; return; }
    string newPass; cout << "Enter new password: "; newPass=inputPassword();
    ifstream fin("accounts.txt"); ofstream fout("temp.txt"); string line;
    while(getline(fin,line)) {
        if(line.empty()) continue;
        stringstream ss(line); string accNoStr,name,pass,type,balance,ph;
        getline(ss,accNoStr,','); getline(ss,name,','); getline(ss,pass,',');
        getline(ss,type,','); getline(ss,balance,','); getline(ss,ph,',');
        if(ph==phone) pass=hashPassword(newPass);
        fout << accNoStr << "," << name << "," << pass << "," << type << "," << balance << "," << ph << endl;
    }
    fin.close(); fout.close();
    remove("accounts.txt"); rename("temp.txt","accounts.txt");
    cout << "Password updated successfully!\n";
}

// ================== Main ==================
int main() {
    srand(time(0));
    int choice;
    while(true) {
        cout << "\n===== Banking System =====\n";
        cout << "1. Create Account\n2. Login\n3. Forgot Password\n4. Exit\nEnter choice: ";
        cin >> choice; cin.ignore();
        if(choice==1) {
            string name, phone, accType; double balance;
            int accNo = generateAccountNumber();
            cout << "Name: "; getline(cin,name);
            cout << "Password: "; string password=inputPassword();
            cout << "Phone (10-digit): "; getline(cin,phone); phone=trim(phone);
            if(phoneExists(phone)) { cout << "Phone exists! Try another.\n"; continue; }
            int typeChoice; do { cout << "Type (1. Savings, 2. Current): "; cin >> typeChoice; cin.ignore(); }
            while(typeChoice!=1 && typeChoice!=2);
            accType=(typeChoice==1)?"Savings":"Current";
            cout << "Initial Balance: "; cin >> balance; cin.ignore();
            if(accType=="Savings" && balance<1000) balance=1000;

            unique_ptr<Account> acc=(accType=="Savings")?unique_ptr<Account>(new SavingsAccount()):unique_ptr<Account>(new CurrentAccount());
            acc->setAccNo(accNo); acc->setName(name); acc->setPassword(hashPassword(password));
            acc->setBalance(balance); acc->setPhone(phone); acc->saveAccount(accType);

            cout << "Hi " << name << ", welcome! Your account number is " << accNo << "\n";
        }
        else if(choice==2) {
            int role; cout << "Role: 1.User 2.Admin: "; cin >> role; cin.ignore();
            if(role==1) {
                int accNo; cout << "Enter Account No: "; cin >> accNo; cin.ignore();
                string accType; auto acc=findAccountByNo(accNo,accType);
                if(acc) {
                    cout << "Password: "; string pass=inputPassword();
                    if(hashPassword(pass)==acc->getPassword()) {
                        cout << "Login successful. Welcome, " << acc->getName() << "!\n";
                        User user(move(acc),accType); user.userMenu();
                    } else cout << "Wrong password!\n";
                } else cout << "Account not found!\n";
            } else if(role==2) {
                cout << "Admin Password: "; string adminPass=inputPassword();
                if(adminPass=="admin123") { Admin admin; admin.adminMenu(); }
                else cout << "Wrong admin password!\n";
            }
        }
        else if(choice==3) resetPasswordByOTP();
        else if(choice==4) { cout << "Exiting...\n"; break; }
    }
    return 0;
}
