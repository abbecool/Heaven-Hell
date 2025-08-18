// #include <functional>
// #include <unordered_map>
// #include <string>
// #include <external/json.hpp> // For JSON parsing

// using json = nlohmann::json;

// class ComponentFactory {
// public:
//     using ComponentCreator = std::function<void*(const json&)>;

//     template <typename T>
//     static void* createComponent(const json& data) {
//         return new T(data);
//     }

//     static void registerComponent(const std::string& name, ComponentCreator creator) {
//         creators[name] = creator;
//     }

//     static void* create(const std::string& name, const json& data) {
//         auto it = creators.find(name);
//         if (it != creators.end()) {
//             return it->second(data);
//         }
//         return nullptr;
//     }

// private:
//     static std::unordered_map<std::string, ComponentCreator> creators;
// };

// // Define the static member
// std::unordered_map<std::string, ComponentFactory::ComponentCreator> ComponentFactory::creators;
