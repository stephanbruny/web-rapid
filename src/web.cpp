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

Web::RouteMethod getMethod(const string & name) {
    string method(name);
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);
    if (method == "POST") return Web::RouteMethod::Post;
    if (method == "UPDATE") return Web::RouteMethod::Update;
    if (method == "PUT") return Web::RouteMethod::Put;
    if (method == "DELETE") return Web::RouteMethod::Delete;
    if (method == "OPTION") return Web::RouteMethod::Option;
    return Web::RouteMethod::Get;
}

void Web::Webserver::setupPrerouting() {
    this->server.set_pre_routing_handler([&](const auto& req, auto& res) {
        auto method = getMethod(req.method);
        auto routeIndex = this->resolvers.find({ method, req.path });
        if (routeIndex == this->resolvers.end()) {
            // 404 not found
            res.status = 404;
            res.set_content("Not found", "text/html");
            return httplib::Server::HandlerResponse::Handled;
        }
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
