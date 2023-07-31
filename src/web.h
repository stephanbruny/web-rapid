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
        string remoteUrl; ///< if set, route will be handled as service resolver
        string remoteDataUrl; ///< if set the data will be received from external source
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
        string staticMount { "/" };
        string templatesDirectory { "files/template" };
        map<RouteReference, Route> routes;
    };

    struct ContentResponse {
        string contentType { "text/html" };
        string content;
        int status { 200 };
    };

    class Resolver {
    public:
        virtual ContentResponse resolve(const httplib::Request & req);
    };

    class ContentResolver : public Resolver {
    private:
        Template::TemplateRenderer & renderer;
        string templateName;
        mustache::data data;
    public:
        ContentResolver(Template::TemplateRenderer & renderer, const string & templateName, mustache::data & data);;

        ContentResponse resolve(const httplib::Request & req) override;
    };

    class ServiceResolver : public Resolver {
    private:
        string serviceUrl;
        string remotePath;
        httplib::Client client;
    public:
        explicit ServiceResolver(const string & url, const string & path = "/");

        ContentResponse resolve(const httplib::Request & req) override;
    };

    class ContentServiceResolver : public Resolver {
    private:
        Template::TemplateRenderer & renderer;
        string templateName;
        string dataUrl;
        ServiceResolver serviceResolver;
    public:
        ContentServiceResolver(Template::TemplateRenderer & renderer, const string & templateName, const string & dataUrl);

        ContentResponse resolve(const httplib::Request & req) override;
    };

    class Webserver {
    private:
        httplib::Server server;
        ServerConfiguration config;
        map<RouteReference, shared_ptr<Resolver>> resolvers;
    protected:
        void setupRoutes();

        void setupPrerouting();
    public:
        explicit Webserver(const ServerConfiguration & conf, Template::TemplateRenderer & renderer);


        void listen();

        bool is_running();

        void close();

        void wait_ready();
    };

    RouteMethod getMethod(const string & name);

    ServerConfiguration getConfigurationFromJson(const string & jsonText);
}

#endif //WEB_RAPID_SERVER_H
