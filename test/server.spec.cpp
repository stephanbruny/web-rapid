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
            std::this_thread::sleep_for(std::chrono::milliseconds{10});

        httplib::Client client("localhost", 8080);
        auto res = client.Get("/");

        cout << "RESULT: " << res->body << endl;

        server.close();
        serverThread.join();
    }
}

