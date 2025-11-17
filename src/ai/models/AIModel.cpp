#include "AIModel.h"

namespace daw::ai::models
{

bool AIModel::loadFromFile(const std::string& path)
{
    modelPath = path;
    // Async loading implementation
    return false;
}

bool AIModel::isLoaded() const noexcept
{
    return loaded;
}

std::string AIModel::getName() const
{
    return modelPath;
}

} // namespace daw::ai::models

