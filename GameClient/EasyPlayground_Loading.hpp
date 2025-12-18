
void EasyPlayground::DeInit()
{
	// Network
	if (network)
		delete network;
	network = nullptr;

	// Players and Main Player
	for (Player* p : players)
		delete p;
	players.clear();
	player = nullptr;

	// HDRs
	if (hdr)
		delete hdr;
	hdr = nullptr;

	// Models
	for (auto& [name, model] : models)
		delete model;
	models.clear();

	// Chunks
	for (Chunk* c : chunks)
		delete c;
	chunks.clear();

	// Camera
	if (camera)
		delete camera;
	camera = nullptr;

	// Engine
	EasyIO::DeInit();
	HDR::DeInit();
	SkyboxRenderer::DeInit();
	ChunkRenderer::DeInit();
	ModelRenderer::DeInit();
	DebugRenderer::DeInit();
}

bool EasyPlayground::Init()
{
	EasyPlayground::DeInit();

	// Inputs
	EasyIO::Init();
	EasyKeyboard::AddListener(this);
	EasyMouse::AddListener(this);

	// Shaders
	ReloadShaders();

	// Assets
	ReloadAssets();

	// ImGUI
	ImGUI_Init();

	return true;
}

void EasyPlayground::ReloadAssets()
{
	if (network)
		delete network;
	network = new ClientNetwork(bm, this);

	for (Player* p : players)
		delete p;
	players.clear();
	player = nullptr;

	for (auto& [name,model] : models)
		delete model;
	models.clear();
	
	Model("MainCharacter") = EasyModel::LoadModel(
		GetRelPath("res/models/Kachujin G Rosales Skin.fbx"),
		{
			GetRelPath("res/models/Standing Idle on Kachujin G Rosales wo Skin.fbx"),
			GetRelPath("res/models/Running on Kachujin G Rosales wo Skin.fbx"),
			GetRelPath("res/models/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx")
		}, glm::vec3(0.0082f) // Y = 1.70m
	);

	Model("1x1cube.dae") = EasyModel::LoadModel(GetRelPath("res/models/1x1cube.dae"));
	Model("items.dae") = EasyModel::LoadModel(GetRelPath("res/models/items.dae"));
	Model("Buildings.dae") = EasyModel::LoadModel(GetRelPath("res/models/Buildings.dae"));
	Model("Walls.dae") = EasyModel::LoadModel(GetRelPath("res/models/Walls.dae"));
	
	if (camera)
		delete camera;
	camera = new FRCamera();

	if (hdr)
		delete hdr;
	hdr = new HDR("defaultLightingHDR");

	ReGenerateMap();
}

void EasyPlayground::ReloadShaders()
{
	std::cout << "[EasyPlayground] ReloadShaders - Reloading shaders...\n";

	HDR::Init();
	DebugRenderer::Init();
	ModelRenderer::Init();
	SkyboxRenderer::Init();
	ChunkRenderer::Init();
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
