#include "gtest/gtest.h"
#include <thread>
#include <utility>
#include "../src/web.h"

using namespace std;

using namespace Web;

namespace {

    TEST(Server, HttpTest) {

        Template::TemplateRenderer renderer;

        map<RouteReference, Route> routes = {
                { { RouteMethod::Get, "/" }, { "/", "../files/example.mustache", "../files/example.json" } }
        };

        Webserver server({ 8080, "0.0.0.0", routes}, renderer);

        auto worker = [&](){
            server.listen();
        };

        thread serverThread(worker);

        while (!server.is_running())
            std::this_thread::sleep_for(std::chrono::milliseconds{1});

        httplib::Client client("localhost", 8080);
        auto res = client.Get("/");

        ASSERT_EQ(res->status, 200);

        auto fail = client.Get("/foobar");

        ASSERT_EQ(fail->status, 404);

        server.close();
        serverThread.join();
    }
}

