#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <windows.h>
#include <iostream>

// Слушатель: печатает ПРОЙДЕН / ПРОВАЛИЛСЯ после каждого теста.
// Работает параллельно с обычным консольным репортером doctest.
struct ResultListener : public doctest::IReporter {
    ResultListener(const doctest::ContextOptions&) {}

    void report_query(const doctest::QueryData&)           override {}
    void test_run_start()                                  override {}
    void test_run_end(const doctest::TestRunStats&)        override {}
    void test_case_start(const doctest::TestCaseData&)     override {}
    void test_case_reenter(const doctest::TestCaseData&)   override {}
    void test_case_exception(const doctest::TestCaseException&) override {}
    void subcase_start(const doctest::SubcaseSignature&)   override {}
    void subcase_end()                                     override {}
    void log_assert(const doctest::AssertData&)            override {}
    void log_message(const doctest::MessageData&)          override {}
    void test_case_skipped(const doctest::TestCaseData&)   override {}

    void test_case_end(const doctest::CurrentTestCaseStats& in) override {
        if (in.failure_flags == 0)
            std::cout << "  --> ПРОЙДЕН\n";
        else
            std::cout << "  --> ПРОВАЛИЛСЯ\n\n";
    }
};
DOCTEST_REGISTER_LISTENER("result_listener", 0, ResultListener);

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    doctest::Context ctx(argc, argv);
    return ctx.run();
}
