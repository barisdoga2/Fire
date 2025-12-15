
void EasyPlayground::ReloadAssets()
{
	players.clear();

	if (model) delete model;
	model = EasyModel::LoadModel(
		GetRelPath("res/models/Kachujin G Rosales Skin.fbx"),
		{
			GetRelPath("res/models/Standing Idle on Kachujin G Rosales wo Skin.fbx"),
			GetRelPath("res/models/Running on Kachujin G Rosales wo Skin.fbx"),
			GetRelPath("res/models/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx")
		}, glm::vec3(0.0082f) // Y = 1.70m
	);

	if (cube_1x1x1) delete cube_1x1x1;
	cube_1x1x1 = EasyModel::LoadModel(GetRelPath("res/models/1x1cube.dae"));

	if (items) delete items;
	items = EasyModel::LoadModel(GetRelPath("res/models/items.dae"));

	if (buildings) delete buildings;
	buildings = EasyModel::LoadModel(GetRelPath("res/models/Buildings.dae"));

	if (walls) delete walls;
	walls = EasyModel::LoadModel(GetRelPath("res/models/Walls.dae"));

	if (camera) delete camera;
	camera = new EasyCamera(display, { 1,4.4,5.8 }, { 1 - 0.15, 4.4 - 0.44,5.8 - 0.895 }, 74.f, 0.01f, 1000.f);

	ReGenerateMap();
}

void EasyPlayground::ReloadShaders()
{
	std::cout << "[EasyPlayground] ReloadShaders - Reloading shaders...\n";

	DebugRenderer::Init();
	ModelRenderer::Init();
	SkyboxRenderer::Init();
	HDR::Init();
	ChunkRenderer::Init();

	if (hdr) delete hdr;
	hdr = new HDR("defaultLightingHDR");
}

void EasyPlayground::ReGenerateMap()
{
	std::cout << "[EasyPlayground] ReGenerateMap - Regenerating map...\n";

	// Cleanup
	for (size_t i = 0u; i < chunks.size(); i++)
		delete chunks.at(i);
	chunks.clear();

	// New Seed
	seed = (int)(glm::linearRand(0.7f, 1.3f) * 10000u);

	// Generate Center
	Chunk::GenerateChunkAt(chunks, { 0,0 }, seed);

	// Make 3x3
	//Chunk::GenerateChunkAt(chunks, { 1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,0 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 0,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { 1,-1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,1 }, seed);
	//Chunk::GenerateChunkAt(chunks, { -1,-1 }, seed);

#if 1
	// Generate some other

#endif

#if 0
	// Add lake
	Chunk::AddLake(chunks, glm::vec2(0.f, 0.f), 30.0f);   // small pond
	Chunk::AddLake(chunks, glm::vec2(100.f, 0.f), 60.0f); // big lake
#endif

	// Load all to GPU
	for (Chunk* c : chunks)
		c->LoadToGPU();
}
