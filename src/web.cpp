#include <cctype>
#include "web.h"

Web::ContentResponse Web::Resolver::resolve(const httplib::Request &req) {
    return { "text/plain", "ok" };
}

Web::ContentResolver::ContentResolver(Template::TemplateRenderer &renderer, const string &templateName,
                                      mustache::data &data) :
        renderer(renderer), templateName(templateName), data(data) {
}

Web::ContentResponse Web::ContentResolver::resolve(const httplib::Request &req) {
    // TODO: add request to data
    string result = renderer.render(this->templateName, this->data);
    // TODO: Caching?
    return { "text/html", result };
}

Web::Webserver::Webserver(const Web::ServerConfiguration &conf, Template::TemplateRenderer &renderer) : config(conf) {
    for(auto & [ref, route] : this->config.routes) {
        shared_ptr<Resolver> resolver;
        if (!route.remoteUrl.empty()) {
            cout << "Service resolver " << route.remoteUrl << " : " << route.path << endl;
            resolver = make_shared<ServiceResolver>(route.remoteUrl);
        } else {
            string templateName = renderer.addTemplateFile(route.templatePath);
            string jsonFileData = File::read(route.dataPath);
            auto data = Template::dataFromJson(jsonFileData);
            resolver = make_shared<ContentResolver>(renderer, templateName, data);
        }
        resolvers.insert({ ref, resolver });
    }
    setupPrerouting();
}

void Web::Webserver::setupRoutes() {
    for (auto & entry : resolvers) {
        auto ref = entry.first;
        auto &resolver = entry.second;
        if (ref.method == RouteMethod::Get) {
            server.Get(ref.path, [&](const httplib::Request & req, httplib::Response & res){
                auto result = resolver->resolve(req);
                res.set_content(result.content, result.contentType);
            });
        }
    }
}

Web::RouteMethod Web::getMethod(const string &name) {
    string method(name);
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    if (method == "POST") return Web::RouteMethod::Post;
    if (method == "UPDATE") return Web::RouteMethod::Update;
    if (method == "PUT") return Web::RouteMethod::Put;
    if (method == "DELETE") return Web::RouteMethod::Delete;
    if (method == "OPTIONS") return Web::RouteMethod::Option;
    return Web::RouteMethod::Get;
}

void Web::Webserver::setupPrerouting() {
    cout << "setup prerouting" << endl;
    this->server.set_mount_point(config.staticMount, config.staticDirectory);

    this->server.set_pre_routing_handler([&](const auto& req, auto& res) {
        cout << "Resolve route " << req.path << endl;
        res.status = 0;
        auto method = getMethod(req.method);
        RouteReference search { method, req.path };

        if (!this->resolvers.count(search)) {
            return httplib::Server::HandlerResponse::Unhandled;
        }

        auto routeIndex = this->resolvers.find(search);
        auto resolver = routeIndex->second;
        auto result = resolver->resolve(req);
        res.status = result.status;
        res.set_content(result.content, result.contentType);

        return httplib::Server::HandlerResponse::Handled;
    });

    this->server.set_error_handler([&](const auto& req, auto& res) {
        // TODO:
        cout << "Error handler called" << endl;
    });

    this->server.set_post_routing_handler([](const auto& req, auto& res) {
        if (res.status == 0) {
            res.status = 404;
            res.set_content("404 - not found", "text/plain");
        }
    });

    this->server.set_exception_handler([](const auto& req, auto& res, std::exception_ptr ep) {
        auto fmt = "<h1>Error 500</h1><p>%s</p>";
        char buf[BUFSIZ];
        try {
            std::rethrow_exception(ep);
        } catch (std::exception &e) {
            snprintf(buf, sizeof(buf), fmt, e.what());
        } catch (...) { // See the following NOTE
            snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }
        res.set_content(buf, "text/html");
        res.status = 500;
    });
    cout << "OK\n";
}

void Web::Webserver::listen() {
    this->server.listen(this->config.host, this->config.port);
}

bool Web::Webserver::is_running() {
    return this->server.is_running();
}

void Web::Webserver::close() {
    this->server.stop();
}

void Web::Webserver::wait_ready() {
    this->server.wait_until_ready();
}

Web::ServerConfiguration Web::getConfigurationFromJson(const string &jsonText) {
    int port = 80;
    string host = "0.0.0.0";
    string filesDir = "files";
    string staticDir = "files/static";
    string staticMount = "/";
    string templatesDir = "files/templates";
    map<RouteReference, Route> routes;

    auto doc = nlohmann::json::parse(jsonText);

    if (!doc.is_object()) {
        throw new runtime_error("Invalid configuration: object expected");
    }

    if (doc["port"].is_number()) port = doc["port"].get<int>();
    if (doc["host"].is_string()) host = doc["host"].get<string>();
    if (doc["files"].is_string()) filesDir = doc["files"].get<string>();
    if (doc["static"].is_string()) staticDir = doc["static"].get<string>();
    if (doc["staticMount"].is_string()) staticMount = doc["staticMount"].get<string>();
    if (doc["templates"].is_string()) templatesDir = doc["templates"].get<string>();

    if (doc["routes"].is_object()) {
        auto routesObject = doc["routes"];
        for (nlohmann::json::iterator it = routesObject.begin(); it != routesObject.end(); ++it) {
            auto method = getMethod(it.key());
            auto methodObject = it.value();
            for (nlohmann::json::iterator routeIt = methodObject.begin(); routeIt != methodObject.end(); ++routeIt) {
                const auto& routePath = routeIt.key();
                auto routeObject = routeIt.value();
                if (!routeObject.is_object()) throw runtime_error("Route must be object");

                RouteReference ref { method, routePath };

                Route result {routePath };
                if (routeObject.contains("template")) result.templatePath = routeObject["template"].get<string>();
                if (routeObject.contains("data")) result.dataPath = routeObject["data"].get<string>();
                if (routeObject.contains("service")) result.remoteUrl = routeObject["service"].get<string>();

                routes.insert(pair(ref, result));
            }
        }
    }

    return {
            port,
            host,
            filesDir,
            staticDir,
            staticMount,
            templatesDir,
            routes
    };
}

Web::ServiceResolver::ServiceResolver(const string &url, const string & path) : serviceUrl(url), remotePath(path) {}

Web::ContentResponse Web::ServiceResolver::resolve(const httplib::Request &req) {
    httplib::Request remoteReq(req);
    remoteReq.path = this->remotePath;
    remoteReq.body = req.body;
    remoteReq.method = req.method;
    remoteReq.params = req.params;
    remoteReq.headers = req.headers;
    httplib::Client client(this->serviceUrl);

    if (!client.is_valid()) {
        throw runtime_error("Invalid service");
    }

    cout << "calling service: " << this->serviceUrl << endl;
    auto response = client.send(remoteReq);
    cout << "service ok " << response->body << endl;
    return { response->get_header_value("Content-Type"), response->body, response->status };
}
