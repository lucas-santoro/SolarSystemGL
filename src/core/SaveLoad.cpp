#include "core/SaveLoad.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>

namespace {

std::string trim(const std::string &s)
{
    const size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    const size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

glm::dvec3 parseDVec3(const std::string &s)
{
    std::istringstream iss(s);
    double x = 0.0, y = 0.0, z = 0.0;
    iss >> x >> y >> z;
    return { x, y, z };
}

glm::vec3 parseVec3(const std::string &s)
{
    std::istringstream iss(s);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    iss >> x >> y >> z;
    return { x, y, z };
}

} // namespace

bool saveSimulation(const std::string &path,
                    const std::vector<CelestialBody> &bodies,
                    const PhysicsSystem &physics)
{
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << std::scientific << std::setprecision(15);

    out << "# SolarSystemGL save file\n";
    out << "version = 1\n";
    out << "timeScale = " << physics.timeScale << "\n\n";

    for (const auto &b : bodies) {
        out << "[body]\n";
        out << "name = "         << b.name << "\n";
        out << "pos_m = "        << b.pos_m.x  << " " << b.pos_m.y  << " " << b.pos_m.z  << "\n";
        out << "vel_m = "        << b.vel_m.x  << " " << b.vel_m.y  << " " << b.vel_m.z  << "\n";
        out << "mass_kg = "      << b.mass_kg  << "\n";
        out << "color = "        << b.color.r  << " " << b.color.g  << " " << b.color.b  << "\n";
        out << "density = "      << b.density  << "\n";
        out << "emissive = "     << b.emissive << "\n";
        out << "displayScale = " << b.displayScale << "\n";
        out << "hasRings = "     << (b.hasRings ? "1" : "0") << "\n";
        out << "hasAtmosphere = "     << (b.hasAtmosphere ? "1" : "0") << "\n";
        out << "atmosphereHeight = "  << b.atmosphereHeight << "\n";
        out << "atmosphereColor = "   << b.atmosphereColor.r << " "
                                      << b.atmosphereColor.g << " "
                                      << b.atmosphereColor.b << "\n";
        out << "atmosphereDensity = " << b.atmosphereDensity << "\n\n";
    }
    return out.good();
}

bool loadSimulation(const std::string &path,
                    std::vector<CelestialBody> &bodies,
                    PhysicsSystem &physics)
{
    std::ifstream in(path);
    if (!in.is_open()) return false;

    std::vector<CelestialBody> staged;
    float                      stagedTimeScale = physics.timeScale;

    CelestialBody *current = nullptr;
    std::string    line;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[body]") {
            staged.emplace_back();
            current = &staged.back();
            continue;
        }

        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        const std::string val = trim(line.substr(eq + 1));

        if (current == nullptr) {
            // Global section (pre-[body])
            if (key == "timeScale") stagedTimeScale = std::stof(val);
            // 'version' and unknown keys silently ignored
        } else {
            if      (key == "name")         current->name         = val;
            else if (key == "pos_m")        current->pos_m        = parseDVec3(val);
            else if (key == "vel_m")        current->vel_m        = parseDVec3(val);
            else if (key == "mass_kg")      current->mass_kg      = std::stod(val);
            else if (key == "color")        current->color        = parseVec3(val);
            else if (key == "density")      current->density      = std::stof(val);
            else if (key == "emissive")     current->emissive     = std::stof(val);
            else if (key == "displayScale") current->displayScale = std::stof(val);
            else if (key == "hasRings")          current->hasRings          = (val == "1" || val == "true");
            else if (key == "hasAtmosphere")     current->hasAtmosphere     = (val == "1" || val == "true");
            else if (key == "atmosphereHeight")  current->atmosphereHeight  = std::stof(val);
            else if (key == "atmosphereColor")   current->atmosphereColor   = parseVec3(val);
            else if (key == "atmosphereDensity") current->atmosphereDensity = std::stof(val);
            // unknown keys silently ignored
        }
    }

    if (staged.empty()) return false;

    // Rebuild GL state for each body. Done BEFORE the atomic swap so that if
    // anything throws (out-of-memory on mesh upload), bodies stays intact.
    for (auto &b : staged) b.recalculateGeometry();

    bodies            = std::move(staged);
    physics.timeScale = stagedTimeScale;
    return true;
}
