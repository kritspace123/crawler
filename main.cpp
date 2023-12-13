#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <thread> ///Работа с несколькими потоками
#include <queue> ///очередь обработки страниц
#include <chrono>
#include <mutex>  ///используется для защиты данных, к которым обращаются несколько потоков
#include <regex> ///регулярные выражения для поиска ссылок
#include <set>

using namespace std;

class WebCrawler{
private:
    int m_NumThreads;
    int m_TotalLinks;
    queue<string> m_UrlQueue; ///Очередь страниц
    chrono::duration<double> m_WorkingHours; ///Хранит время работы
    set<string> VisitedPages; ///Все ссылки которые мы посетили
    mutex m_UrlQueueMutex; ///предотвращает возможные проблемы с одновременным доступом.


    mutex m_VisitedPagesMutex; ///======
public:
    WebCrawler(int NumThreads);
    void Start(string Initial_Url);
    void Crawl(string ur);
    void ProcessUrl();
    string CopyingFile(string url);
};
WebCrawler::WebCrawler(int NumThreads): m_NumThreads(NumThreads), m_TotalLinks(0){};
void WebCrawler::Crawl(string url) {
    string fileName = CopyingFile(url);
    ifstream F(fileName);
    string line;
    regex linkRegex("<a\\s+href=\"file://([^\"]+)\">");
    while (getline(F, line)) {
        smatch match;
        if (regex_search(line, match, linkRegex)) {
            string URL = match[1];
            lock_guard<mutex> lock(m_VisitedPagesMutex); ///=====
            if (VisitedPages.count(URL) <= 0) {
                lock_guard<mutex> lockQueue(m_UrlQueueMutex);
                m_UrlQueue.push(URL);
                VisitedPages.insert(URL);
            }
        }
    }
}void WebCrawler::ProcessUrl() {
    while (true) {
        unique_lock<mutex> lock(m_UrlQueueMutex);  ///блокирует мьютекс
        if (m_UrlQueue.empty()) { ///Проверяем, пуста ли очередь URL
            lock.unlock();
            break;
        }

        string url = m_UrlQueue.front();
        m_UrlQueue.pop();
        lock.unlock(); ///Это позволяет другим потокам получить доступ к очереди.

        Crawl(url);
        m_TotalLinks ++;
    }
}
string WebCrawler::CopyingFile(string url) {
    regex pattern("(.*\\.html)");
    string replacement= "site\\$1";

    string result = regex_replace(url, pattern, replacement);
    ifstream F(url, ios::binary);
    if (!F) {
        cout << url;
        cerr << "Ошибка открытия файла1" << endl;
        exit(1);
    }

    ofstream Out(result, ios::binary);
    if (!Out.is_open()) {
        cerr << "Ошибка открытия файла2   " <<strerror(errno) << endl;
        exit(1);
    }
    Out << F.rdbuf();
    F.close();
    Out.close();
    return result;
}
void WebCrawler::Start(string Url) {
    auto start = chrono::high_resolution_clock::now(); ///начинаем отсчёт врмени

    /// Первая обработка начальной страницы
    Crawl(Url);
    VisitedPages.insert(Url);
    m_TotalLinks ++; //ат

    /// Запуск потоков для обработки остальных страниц
    vector<thread> threads;
    for (int i = 0; i < m_NumThreads; ++i) {
        threads.emplace_back(&WebCrawler::ProcessUrl, this);
    }

    ///  Дождемся завершения всех потоков
    for (auto &thread : threads) {
        thread.join();
    }

    auto end = chrono::high_resolution_clock::now(); ///Заканчиваем отсчёт времени
    m_WorkingHours = end - start;
    cout << "Количество посещенных страниц: " << m_TotalLinks << endl;
    cout << "Время работы: " << m_WorkingHours.count() << " c." << endl;
}

int main() {
    system("chcp 65001");
    string InitialUrl = "index.html";
    WebCrawler Crawl(8);
    Crawl.Start(InitialUrl);
    return 0;
}
