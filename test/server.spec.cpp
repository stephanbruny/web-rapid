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

        ServerConfiguration conf {
            8080,
            "0.0.0.0",
            "files",
            "files/static",
            "files/templates",
            routes
        };

        Webserver server(conf, renderer);

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

    TEST(Server, ConfigFromJson) {
        const char TEST_CONFIG[] = R"json(
        {
            "port": 1337,
            "files":  "files",
            "static": "files/static",
            "templates": "templates",
            "routes": {
                "get": {
                    "/": {
                        "template": "home.mustache",
                        "data": "data/home.json"
                    },
                    "/foo": {
                        "template": "foo.mustache",
                        "data": "data/foo.json"
                    }
                },
                "post": {
                    "/": { "template": "foobar" }
                }
            }
        }
        )json";

        auto conf = Web::getConfigurationFromJson(TEST_CONFIG);

        ASSERT_EQ(conf.port, 1337);
        ASSERT_STREQ(conf.host.c_str(), "0.0.0.0"); // default value
        ASSERT_STREQ(conf.filesDirectory.c_str(), "files"); // default value
        ASSERT_STREQ(conf.staticDirectory.c_str(), "files/static");
        ASSERT_STREQ(conf.templatesDirectory.c_str(), "templates");

        ASSERT_EQ(conf.routes.count({ Web::RouteMethod::Get, "/" }), 1);
        ASSERT_EQ(conf.routes.count({ Web::RouteMethod::Get, "/foo" }), 1);
        ASSERT_EQ(conf.routes.count({ Web::RouteMethod::Post, "/" }), 1);
    }

    TEST(Server, StaticFiles) {
        Template::TemplateRenderer renderer;

        map<RouteReference, Route> routes = {
                { { RouteMethod::Get, "/" }, { "/", "../files/example.mustache", "../files/example.json" } }
        };

        ServerConfiguration conf {
                8080,
                "0.0.0.0",
                "../files",
                "../files/static",
                "../files/templates",
                routes
        };

        Webserver server(conf, renderer);

        auto worker = [&](){
            server.listen();
        };

        thread serverThread(worker);

        while (!server.is_running())
            std::this_thread::sleep_for(std::chrono::milliseconds{1});

        httplib::Client client("localhost", 8080);
        auto res = client.Get("/test.png");
        client.Get("/static/test.png");

        auto fileContents = File::read("../files/static/test.png");

        ASSERT_EQ(res->status, 200);

        ASSERT_STREQ(res->body.c_str(), fileContents.c_str());

        server.close();
        serverThread.join();
    }
}

