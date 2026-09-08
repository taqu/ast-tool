#include "session_manager.h"

namespace session {

Session SessionManager::create(const std::string& user) {
    Session s{static_cast<int>(sessions_.size()), user + "-session", user};
    sessions_[s.token] = s;
    return s;
}

Session SessionManager::get(const std::string& token) {
    auto it = sessions_.find(token);
    if (it == sessions_.end()) return Session{};
    return it->second;
}

bool SessionManager::update(const Session& session) {
    sessions_[session.token] = session;
    return true;
}

bool SessionManager::destroy(const std::string& token) {
    return sessions_.erase(token) > 0;
}

} // namespace session
