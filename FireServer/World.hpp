#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

#include <EasyNet.hpp>
#include <EasySerializer.hpp>
#include "Net.hpp"

#if 0
constexpr float WORLD_SIZE = 10000.f;
constexpr float HALF_WORLD = WORLD_SIZE * 0.5f;
constexpr float CHUNK_SIZE = 250.f;
constexpr float VIEW_RADIUS = 100.f;
#else
constexpr float WORLD_SIZE = 1000.f;
constexpr float HALF_WORLD = WORLD_SIZE * 0.5f;
constexpr float CHUNK_SIZE = 250.f;
constexpr float VIEW_RADIUS = 100.f;
#endif

namespace b2 {
    enum class ShapeType : uint8_t { Polygon, Circle };

    class pWorld : public EasySerializeable {
    public:
        glm::vec2 gravity{};

        pWorld() : EasySerializeable(static_cast<PacketID_t>(WORLD_DEF))
        {

        }

        void Serialize(EasySerializer* ser) override
        {
            ser->Put(gravity);
        }
    };
    REGISTER_PACKET(pWorld, WORLD_DEF);

    class pMaterial : public EasySerializeable {
    public:
        float friction{};
        float restitution{};
        float rollingResistance{};
        float tangentSpeed{};
        std::string b{};
        std::vector<float> a{};

        pMaterial() : EasySerializeable(static_cast<PacketID_t>(MATERIAL_DEF))
        {

        }

        void Serialize(EasySerializer* ser) override
        {
            ser->Put(friction);
            ser->Put(restitution);
            ser->Put(rollingResistance); 
            ser->Put(tangentSpeed); 
            ser->Put(a);
            ser->Put(b);
        }
    };
    REGISTER_PACKET(pMaterial, MATERIAL_DEF);

    class pShape {
    public:
        pMaterial material;
        virtual const ShapeType Type() const = 0;

    };

    class pCircleShape : public pShape , public EasySerializeable {
    public:
        float radius;
        const ShapeType Type() const override { return ShapeType::Circle; }

        pCircleShape() : EasySerializeable(static_cast<PacketID_t>(CIRCLE_DEF)), radius(1.f)
        {


        }

        void Serialize(EasySerializer* ser)
        {
            ser->Put(material);
            ser->Put(radius);
        }
    };
    REGISTER_PACKET(pCircleShape, CIRCLE_DEF);

    class pPolyShape : public pShape, public EasySerializeable {
    public:
        std::vector<glm::vec2> vertices;
        const ShapeType Type() const override { return ShapeType::Polygon; }

        pPolyShape() : EasySerializeable(static_cast<PacketID_t>(POLY_DEF))
        {


        }

        void Serialize(EasySerializer* ser)
        {
            ser->Put(material);
            ser->Put(vertices);
        }
    };
    REGISTER_PACKET(pPolyShape, POLY_DEF);

    struct pBodyData {
        uint32_t id;
        glm::vec2 position;
        float angle;
        std::vector<std::unique_ptr<pShape>> shapes;
    };

    struct pWorldData {
        glm::vec2 gravity;
        std::vector<pBodyData> bodies;
    };

    static b2WorldId CreateWorldFromData(const pWorldData& data)
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { data.gravity.x, data.gravity.y };
        b2WorldId world = b2CreateWorld(&worldDef);

        for (auto& b : data.bodies)
        {
            b2BodyDef bd = b2DefaultBodyDef();
            bd.position = { b.position.x, b.position.y };
            b2BodyId body = b2CreateBody(world, &bd);

            for (auto& s : b.shapes)
            {
                b2ShapeDef sd = b2DefaultShapeDef();
                sd.material.friction = s.get()->material.friction;
                sd.material.restitution = s.get()->material.restitution;
                sd.material.rollingResistance = s.get()->material.rollingResistance;
                sd.material.tangentSpeed = s.get()->material.tangentSpeed;

                if (auto c = dynamic_cast<pCircleShape*>(s.get()))
                {
                    b2Circle circle = { {0,0}, c->radius };
                    b2CreateCircleShape(body, &sd, &circle);
                }
                else if (auto p = dynamic_cast<pPolyShape*>(s.get()))
                {
                    std::vector<b2Vec2> verts;
                    verts.reserve(p->vertices.size());
                    for (auto& v : p->vertices)
                        verts.push_back({ v.x, v.y });

                    b2Hull hull = b2ComputeHull(verts.data(), (int)verts.size());
                    b2Polygon poly = b2MakePolygon(&hull, 0.0f);

                    b2CreatePolygonShape(body, &sd, &poly);
                }
            }
        }
        return world;
    }
}


struct WChunk {
    uint32_t id;
    glm::ivec2 gridPos;
    std::unordered_set<EasyPeer*> subscribers;

    void addSubscriber(EasyPeer* p) { subscribers.insert(p); }
    void removeSubscriber(EasyPeer* p) { subscribers.erase(p); }

    void broadcast(const std::string& msg);
    glm::vec2 getWorldCenter();

};

class World {
public:
    b2WorldId world;
    World();
    WChunk* getChunkAt(const glm::vec2& pos);
    std::vector<WChunk*> getChunksInRadius(const glm::vec2& pos, float radius);
    void updatePeerSubscription(EasyPeer& peer);
    void Update(double _dt);
private:
    std::vector<WChunk> chunks;
    uint32_t widthChunks, heightChunks;

};
