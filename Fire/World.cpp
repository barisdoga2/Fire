#include "World.hpp"

#include <cmath>
#include <iostream>
#include <algorithm>

#include <glm/glm.hpp>

using namespace b2;



World::World() 
{
    // Init Box2D
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, -10.0f };
        world = b2CreateWorld(&worldDef);

        b2BodyDef groundBodyDef = b2DefaultBodyDef();
        groundBodyDef.position = { 0.0f, -10.0f };
        b2BodyId groundId = b2CreateBody(world, &groundBodyDef);
        b2Polygon groundBox = b2MakeBox(50.0f, 10.0f);
        b2ShapeDef groundShapeDef = b2DefaultShapeDef();
        b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    }

    widthChunks = static_cast<uint32_t>(WORLD_SIZE / CHUNK_SIZE);
    heightChunks = widthChunks;
    chunks.reserve(widthChunks * heightChunks);

    for (uint32_t y = 0; y < heightChunks; ++y)
    {
        for (uint32_t x = 0; x < widthChunks; ++x) 
        {
            Chunk c;
            c.id = y * widthChunks + x;
            c.gridPos = { (int)x, (int)y };
            chunks.push_back(std::move(c));
        }
    }
}

void World::Update(double _dt)
{
    b2World_Step(world, (float)(_dt / 1000.0), 4);
}

Chunk* World::getChunkAt(const glm::vec2& pos)
{
    int cx = (int)((pos.x + HALF_WORLD) / CHUNK_SIZE);
    int cy = (int)((pos.y + HALF_WORLD) / CHUNK_SIZE);

    cx = std::clamp(cx, 0, (int)widthChunks - 1);
    cy = std::clamp(cy, 0, (int)heightChunks - 1);

    return &chunks[cy * widthChunks + cx];
}

std::vector<Chunk*> World::getChunksInRadius(const glm::vec2& pos, float radius) 
{
    std::vector<Chunk*> result;

    int minX = std::max(0, (int)((pos.x - radius + HALF_WORLD) / CHUNK_SIZE));
    int maxX = std::min((int)widthChunks - 1, (int)((pos.x + radius + HALF_WORLD) / CHUNK_SIZE));
    int minY = std::max(0, (int)((pos.y - radius + HALF_WORLD) / CHUNK_SIZE));
    int maxY = std::min((int)heightChunks - 1, (int)((pos.y + radius + HALF_WORLD) / CHUNK_SIZE));

    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            auto& chunk = chunks[y * widthChunks + x];

            glm::vec2 chunkCenter{-HALF_WORLD + (x + 0.5f) * CHUNK_SIZE, -HALF_WORLD + (y + 0.5f) * CHUNK_SIZE};

            if (glm::distance(chunkCenter, pos) <= radius + CHUNK_SIZE * 0.75f)
                result.push_back(&chunk);
        }
    }
    for (auto c : result)
        std::cout <<  "chunk " << c->getWorldCenter().x << " " << c->getWorldCenter().y << "\n";
    return result;
}

void World::updatePeerSubscription(EasyPeer& peer)
{
    auto newChunks = getChunksInRadius(peer.player.position, VIEW_RADIUS);

    std::unordered_set<uint32_t> newSet;
    for (auto* c : newChunks)
    {
        newSet.insert(c->id);
        if (!peer.player.subscribedChunks.count(c->id))
        {
            c->addSubscriber(&peer);
            peer.player.subscribedChunks.insert(c->id);
        }
    }

    // Remove chunks no longer in range
    for (auto it = peer.player.subscribedChunks.begin(); it != peer.player.subscribedChunks.end();)
    {
        if (!newSet.count(*it))
        {
            chunks[*it].removeSubscriber(&peer);
            it = peer.player.subscribedChunks.erase(it);
        }
        else ++it;
    }
}

void Chunk::broadcast(const std::string& msg)
{
    //for (auto* peer : subscribers)
    //    peer->receiveMessage(msg);
}

glm::vec2 Chunk::getWorldCenter()
{
    glm::vec2 ret = { 0,0 };

    ret.x = gridPos.x * CHUNK_SIZE + CHUNK_SIZE / 2.f - HALF_WORLD;
    ret.y = gridPos.y * CHUNK_SIZE + CHUNK_SIZE / 2.f - HALF_WORLD;

    return ret;
}
