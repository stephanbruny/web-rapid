//
// Created by Stephan Bruny on 25.07.23.
//

#ifndef WEB_RAPID_SERVER_H
#define WEB_RAPID_SERVER_H

#include <thread>
#include <string>

#include "file.h"
#include "template.h"
#include "httplib.h"
#include "json.hpp"

using namespace std;

namespace Web {

    struct Route {
        string path;
        string templatePath;
        string dataPath;
    };

    enum class RouteMethod {
        Get,
        Post,
        Put,
        Update,
        Delete,
        Option
    };

    struct RouteReference {
        RouteMethod method;
        string path;

        inline bool operator ==(const RouteReference & b) const {
            return b.path == path && b.method == method;
        }

        inline bool operator <(const RouteReference & b) const {
            return make_pair(method, path) < make_pair(b.method, b.path);
        }
    };

    struct ServerConfiguration {
        int port;
        string host { "0.0.0.0" };
        string filesDirectory { "files" };
        string staticDirectory { "files/static" };
        string templatesDirectory { "files/template" };
        map<RouteReference, Route> routes;
    };

    struct ContentResponse {
        string contentType { "text/html" };
        string content;
    };

    class ContentResolver {
    private:
        Template::TemplateRenderer & renderer;
        string templateName;
        mustache::data data;
    public:
        ContentResolver(Template::TemplateRenderer & renderer, const string & templateName, mustache::data & data);;

        ContentResponse resolve(const httplib::Request & req);
    };

    class Webserver {
    private:
        httplib::Server server;
        ServerConfiguration config;
        map<RouteReference, shared_ptr<ContentResolver>> resolvers;
    protected:
        void setupRoutes();

        void setupPrerouting();
    public:
        explicit Webserver(const ServerConfiguration & conf, Template::TemplateRenderer & renderer);


        void listen();

        bool is_running();

        void close();
    };

    RouteMethod getMethod(const string & name);

    ServerConfiguration getConfigurationFromJson(const string & jsonText);
}

#endif //WEB_RAPID_SERVER_H
