#include <cctype>
#include "web.h"

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
        string templateName = renderer.addTemplateFile(route.templatePath);
        string jsonFileData = File::read(route.dataPath);
        auto data = Template::dataFromJson(jsonFileData);
        auto resolver = make_shared<ContentResolver>(renderer, templateName, data);
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
    this->server.set_pre_routing_handler([&](const auto& req, auto& res) {
        res.status = 200;
        auto method = getMethod(req.method);
        RouteReference search { method, req.path };
        if (!this->resolvers.count(search)) {
            cout << "not found" << endl;
            res.status = 404;
            res.set_content("Not found", "text/html");
            return httplib::Server::HandlerResponse::Handled;
        }
        auto routeIndex = this->resolvers.find(search);
        auto resolver = routeIndex->second;
        auto result = resolver->resolve(req);
        res.set_content(result.content, result.contentType);

        return httplib::Server::HandlerResponse::Handled;
    });
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

Web::ServerConfiguration Web::getConfigurationFromJson(const string &jsonText) {
    int port = 80;
    string host = "0.0.0.0";
    string filesDir = "files";
    string staticDir = "files/static";
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

                routes.insert(pair(ref, result));
            }
        }
    }

    return {
            port,
            host,
            filesDir,
            staticDir,
            templatesDir,
            routes
    };
}
