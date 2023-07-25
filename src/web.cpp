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
    setupRoutes();
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

void Web::Webserver::listen() {
    this->server.listen(this->config.host, this->config.port);
}

bool Web::Webserver::is_running() {
    return this->server.is_running();
}

void Web::Webserver::close() {
    this->server.stop();
}
