#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <aml-psdk/game_sa/plugin.h>

#include <cstdint>
#include <cstring>

MYMODCFG(net.psdk.samod.freeclothes, SA Android Free Clothes, 1.3, Jean7z)

static ConfigEntry *cfgUnlockAll, *cfgFreePrice, *cfgAllShops;

/* Base de libGTASA.so (relocada por ASLR) para re-resolver el script
 * space en caliente: al cargar partida el juego puede reasignar el buffer
 * de globals SCM, y la copia cacheada de ON_MOD_LOAD quedaría stale. */
static uintptr_t g_pGame = 0;

/* ---- Armario de casa: desbloquear las 6 tiendas extra ----
 * CWidgetListShop::AddItem es virtual — el caller no aparece en el
 * objdump. Se resolvió por disasm de CollectParameters@0x3f1fcc que
 * los globals SCM base son *(pGame+0x84A0F0) y el índice es byte offset.
 * Forzando los gates del scriptv1.img a 1 en LoadShop("bought") (que el
 * armario de casa llama ANTES de armar sus filas), el SCM añade las
 * filas SHOP2-7 él mismo con su dispatch nativo por índice.
 */
static uintptr_t g_scriptSpace = 0; /* SCM globals: *(int*)(g_scriptSpace + idx)
                                       idx = byte offset (type 0x02, handler
                                       3f2030: ldr w,[x10,x13] sin *4) */

/* ---- Gates del armario en el SCM (scriptv1.img, decodificado 0x83240+):
 * Cada tienda se añade con  if global[X]==1 → AddItem("SHOPn").
 *   SHOP1=0x3D80  SHOP2=0x3D84  SHOP3=0x3D88  SHOP4=0x3D8C
 *   SHOP5=0x3D90  SHOP6=0x3D94  SHOP7=0x3D98+0x3D9C+0x3DA0+0x3DAC+
 *                  0x3DA4+0x3DA8  (+0x3DB0,0x3DB4,0xB0F8)
 * En partida solo SHOP1 (compra de la historia en Binco) y REMCLT (0xB104,
 * quitar prendas) están a 1. Forjándolos a 1 desde LoadShop("bought") — que
 * el armario de casa llama ANTES de armar sus filas — el SCM añade SHOP2-7
 * él mismo → dispatch nativo por índice del script.
 *
 * Dirección de un global SCM (mobile 2.10) según CollectParameters@3f1fcc:
 *   type 0x02 → addr = *(pGame+0x84A0F0) + idx   (ldr w13,[x10,x13], idx=byte)
 *   (0x84B810 se usa en handlers con lsl#2 — arrays/otra cosa, NO globals)
 * Huella vanilla (debe cumplirse): gate 0x3D80 == 1, 0x3D84 == 0, 0xB104 == 1. */
static void forgeShopGates()
{
    static const unsigned gateVars[] = {
        0x3D80, 0x3D84, 0x3D88, 0x3D8C, 0x3D90, 0x3D94,
        0x3D98, 0x3D9C, 0x3DA0, 0x3DA4, 0x3DA8, 0x3DAC,
        0x3DB0, 0x3DB4, 0xB0F8,
    };
    static const unsigned remcltVar = 0xB104;

    /* Re-resuelve el script space en caliente: al cargar partida el juego
     * puede reasignar el buffer de globals SCM (el valor cacheado de
     * ON_MOD_LOAD quedaría stale y forjaríamos en memoria muerta). */
    if(g_pGame)
    {
        uintptr_t fresh = *(uintptr_t*)(g_pGame + 0x84A0F0);
        if(fresh && fresh != g_scriptSpace)
        {
            logger->Info("DBG script space moved: 0x%lX -> 0x%lX (re-resolved)",
                         (unsigned long)g_scriptSpace, (unsigned long)fresh);
            g_scriptSpace = fresh;
        }
    }

    if(!g_scriptSpace) return;

    logger->Info("DBG globals: space=0x%lX", (unsigned long)g_scriptSpace);

    int g1  = *(int*)(g_scriptSpace + 0x3D80);
    int g2  = *(int*)(g_scriptSpace + 0x3D84);
    int rem = *(int*)(g_scriptSpace + remcltVar);
    logger->Info("DBG   g[3D80]=%d g[3D84]=%d g[B104]=%d %s",
                 g1, g2, rem,
                 (g1 == 1 && g2 == 0 && rem == 1) ? "  <<< FIRMA OK" : "");

    /* Forja igualmente los 15 gates a 1 (huella conocida). */
    for(unsigned v : gateVars)
        *(int*)(g_scriptSpace + v) = 1;
    *(int*)(g_scriptSpace + remcltVar) = 1;
    logger->Info("DBG FORGED 15 shop gates + REMCLT (space=0x%lX)",
                 (unsigned long)g_scriptSpace);
}

/* ---- BSS globals resolved at runtime from GOT (arm64 only) ----
 *
 * CShopping::LoadShop loads ONE shop section from shopping.dat into a
 * fixed-size int array.  The safehouse wardrobe only calls LoadShop with
 * the player's "current" shop, so only that shop's items appear.
 *
 * By hooking LoadShop we call the original for ALL six clothing shops,
 * accumulate every item into a temp buffer, and write the merged result
 * back — the wardrobe then displays everything.
 *
 * GOT offsets derived from arm64 disassembly of LoadShop (2.10):
 *   0x850e88 → &ms_numItemsInShop  (int,  reset to 0 each call)
 *   0x850278 → &ms_shopContents   (int[], item IDs, stride 4)
 *   0x84c050 → &ms_shopLoaded     (char[], cache string)
 */
static int*  g_numItemsInShop = nullptr;
static int*  g_shopContents   = nullptr;
static char* g_shopLoaded     = nullptr;
static char* g_bHasBought     = nullptr;

static const char* g_clothingShops[] = {
    "CSchp",   /* Binco         */
    "CSsprt",  /* ProLaps       */
    "LACS1",   /* Sub Urban     */
    "clothgp", /* ZIP           */
    "Csdesgn", /* Victim        */
    "Csexl",   /* Didier Sachs  */
};
static constexpr int kNumShops  = sizeof(g_clothingShops) / sizeof(g_clothingShops[0]);
/* ms_shopContents is int[300] (1200 B) — never write past it. */
static constexpr int kMaxMerged = 300;
/* ms_bHasBought is 560 bytes (0x230) of ownership flags. */
static constexpr int kBoughtSize = 560;

/* CShopping::HasPlayerBought(itemId)
 * ==================================
 * Force true → every item shows as "owned" in the wardrobe.
 *
 * 2.10 arm64: _ZN9CShopping15HasPlayerBoughtEj @ 0x430228 (dynsym) */
DECL_HOOKb(CShopping__HasPlayerBought, unsigned int itemId)
{
    if(cfgUnlockAll && cfgUnlockAll->GetBool()) return true;
    return CShopping__HasPlayerBought(itemId);
}

/* CShopping::GetPrice(itemId)
 * ===========================
 * Return 0 → buying costs nothing.
 *
 * 2.10 arm64: _ZN9CShopping8GetPriceEj @ 0x42f348 (dynsym) */
DECL_HOOKi(CShopping__GetPrice, unsigned int itemId)
{
    if(cfgFreePrice && cfgFreePrice->GetBool()) return 0;
    return CShopping__GetPrice(itemId);
}

/* CShopping::LoadShop(shopName)
 * =============================
 * Original loads one section (shopName) from data/shopping.dat into
 * ms_shopContents[0..N-1] and sets ms_numItemsInShop = N.
 * It also copies shopName → ms_shopLoaded and returns early if the
 * same name is requested again (cache).
 *
 * Special case: when shopName == "bought." (the safehouse wardrobe),
 * the original iterates ALL prices and copies only items whose
 * ms_bHasBought[key] == 1 into ms_shopContents — so the wardrobe only
 * shows clothes the player already owns. We force the whole bitmap to 1
 * first, and then merge all six clothing shops anyway, so the wardrobe
 * lists everything.
 *
 * Our hook:
 *   1. Real shop (anything but "bought") → vanilla single-section load:
 *      each store shows its own content. (The previous version merged all
 *      six shops on EVERY call, so every store displayed Binco's items.)
 *   2. Only "bought" (safehouse wardrobe): bypass the cache by clearing
 *      g_shopLoaded before each call, load all six shops into a local
 *      buffer, write the merged result back and restore g_shopLoaded.
 *
 * 2.10 arm64: _ZN9CShopping8LoadShopEPKc @ 0x42eb04 (dynsym) */
DECL_HOOKv(CShopping__LoadShop, const char* shopName)
{
    if(!cfgAllShops || !cfgAllShops->GetBool() ||
       !g_numItemsInShop || !g_shopContents)
    {
        CShopping__LoadShop(shopName);
        return;
    }

    /* Tienda real (Binco/ProLaps/Didier…): vanilla, sección única. Merge
     * SOLO en el armario ("bought") para no contaminar el contenido de
     * cada tienda (el merge global hacía que todas enseñasen Binco). */
    if(!shopName || strcasecmp(shopName, "bought") != 0)
    {
        CShopping__LoadShop(shopName);
        return;
    }

    /* Safehouse wardrobe ("bought" special path): mark everything as
       owned so the original's per-key filter lets every item through.
       NB: the special string at 0x74428a is "bought" — NO trailing dot. */
    if(g_bHasBought && cfgUnlockAll && cfgUnlockAll->GetBool())
    {
        memset(g_bHasBought, 1, kBoughtSize);
        logger->Info("LoadShop('bought'): all %d ownership flags set", kBoughtSize);
    }

    /* Forja los gates del SCM AQUÍ, ANTES de que el script arme las filas
     * del armario: LoadShop("bought") lo llama el setup del armario de la
     * casa ANTES de evaluar los if global[X]==1. Forjar en AddItem(SHOP1)
     * llegaba tarde — el pase ya había evaluado los gates de SHOP2-7. */
    forgeShopGates();

    static int merged[kMaxMerged];
    int total = 0;

    for(int i = 0; i < kNumShops; i++)
    {
        /* Clear cache string so the original won't short-circuit. */
        if(g_shopLoaded) g_shopLoaded[0] = '\0';

        CShopping__LoadShop(g_clothingShops[i]);

        int n = *g_numItemsInShop;
        logger->Info("LoadShop merge[%d] '%s': %d items", i, g_clothingShops[i], n);
        if(n > 0 && total + n <= kMaxMerged)
        {
            memcpy(&merged[total], g_shopContents, (size_t)n * sizeof(int));
            total += n;
        }
        else
        {
            logger->Error("LoadShop merge[%d] '%s': skipped (%d items, total %d, cap %d)",
                          i, g_clothingShops[i], n, total, kMaxMerged);
        }
    }

    logger->Info("LoadShop: merged %d items across %d shops (requested '%s')",
                 total, kNumShops, shopName ? shopName : "(null)");

    /* Write the combined set back. */
    memcpy(g_shopContents, merged, (size_t)total * sizeof(int));
    *g_numItemsInShop = total;

    /* Restore the requested name so the game's own cache logic stays
       consistent with whatever it expects ms_shopLoaded to hold. */
    if(g_shopLoaded && shopName)
    {
        size_t len = strlen(shopName);
        if(len > 23) len = 23;
        memcpy(g_shopLoaded, shopName, len);
        g_shopLoaded[len] = '\0';
    }
}

/* CShopping::Load()
 * ===================
 * Called at startup and when a savegame is loaded; it reads the
 * ms_bHasBought flags back from the save. We re-apply "everything owned"
 * right after so the wardrobe never reverts to the saved subset.
 * Also re-forges the SCM wardrobe gates: the savegame carries the vanilla
 * values of those globals (only SHOP1 on), so after loading, the wardrobe
 * at any safehouse would show just Binco until some LoadShop("bought")
 * re-fires. Forging right here covers that case before the player even
 * opens the wardrobe.
 *
 * 2.10 arm64: _ZN9CShopping4LoadEv @ 0x580f28 (dynsym) */
DECL_HOOKv(CShopping__Load)
{
    CShopping__Load();

    if(g_bHasBought && cfgUnlockAll && cfgUnlockAll->GetBool())
    {
        memset(g_bHasBought, 1, kBoughtSize);
        logger->Info("CShopping::Load(): re-applied %d ownership flags", kBoughtSize);
    }

    if(cfgAllShops && cfgAllShops->GetBool())
        forgeShopGates();
}

/* CShopping::Init()
 * ===================
 * Called during game init; it memsets ms_bHasBought (560 bytes) to 0,
 * wiping whatever we set in ON_MOD_LOAD. Hook it and re-apply.
 *
 * 2.10 arm64: _ZN9CShopping4InitEv @ 0x42e008 (dynsym) */
DECL_HOOKv(CShopping__Init)
{
    CShopping__Init();

    if(g_bHasBought && cfgUnlockAll && cfgUnlockAll->GetBool())
    {
        memset(g_bHasBought, 1, kBoughtSize);
        logger->Info("CShopping::Init(): re-applied %d ownership flags", kBoughtSize);
    }
}

ON_MOD_LOAD()
{
    logger->SetTag("FreeClothes");

    cfgUnlockAll = cfg->Bind("UnlockAll", true, "Clothes");
    cfgFreePrice = cfg->Bind("FreePrice", true, "Clothes");
    cfgAllShops  = cfg->Bind("AllShops",  true, "Clothes");

    uintptr_t pGame = aml->GetLib("libGTASA.so");
    if(!pGame)
    {
        logger->Error("libGTASA.so not found!");
        return;
    }
    g_pGame = pGame;

    /* Resolve function symbols via dlsym. */
    uintptr_t symHasBought = aml->GetSym(pGame, "_ZN9CShopping15HasPlayerBoughtEj");
    uintptr_t symGetPrice  = aml->GetSym(pGame, "_ZN9CShopping8GetPriceEj");
    uintptr_t symLoadShop  = aml->GetSym(pGame, "_ZN9CShopping8LoadShopEPKc");
    uintptr_t symLoad      = aml->GetSym(pGame, "_ZN9CShopping4LoadEv");
    uintptr_t symInit      = aml->GetSym(pGame, "_ZN9CShopping4InitEv");

    if(!symHasBought || !symGetPrice || !symLoadShop)
    {
        logger->Error("Symbols: HasBought=0x%lX Price=0x%lX LoadShop=0x%lX",
                      (unsigned long)symHasBought, (unsigned long)symGetPrice,
                      (unsigned long)symLoadShop);
        return;
    }

    /* Resolve BSS globals via GOT entries (arm64 only).
       At runtime these already contain the relocated addresses. */
    uintptr_t gotNumItems  = *(uintptr_t*)(pGame + 0x850e88);
    uintptr_t gotContents = *(uintptr_t*)(pGame + 0x850278);
    uintptr_t gotLoaded   = *(uintptr_t*)(pGame + 0x84c050);
    uintptr_t gotBought   = *(uintptr_t*)(pGame + 0x84e7b8);

    logger->Info("GOT: numItems=0x%lX contents=0x%lX loaded=0x%lX bought=0x%lX",
                 (unsigned long)gotNumItems, (unsigned long)gotContents,
                 (unsigned long)gotLoaded, (unsigned long)gotBought);

    if(gotNumItems && gotContents)
    {
        g_numItemsInShop = (int*)gotNumItems;
        g_shopContents   = (int*)gotContents;
        g_shopLoaded     = (char*)gotLoaded;
        g_bHasBought     = (char*)gotBought;
    }
    else
    {
        logger->Error("GOT resolve failed — AllShops will be OFF");
    }

    /* Install hooks. */
    HOOK(CShopping__HasPlayerBought, symHasBought);
    HOOK(CShopping__GetPrice, symGetPrice);
    if(g_numItemsInShop && g_shopContents)
        HOOK(CShopping__LoadShop, symLoadShop);
    if(g_bHasBought && symLoad)
        HOOK(CShopping__Load, symLoad);
    if(g_bHasBought && symInit)
        HOOK(CShopping__Init, symInit);

    /* SCM globals (type 0x02): *(int*)(base + idx), idx = byte offset.
     * CollectParameters@0x3f1fcc: ldr x10,[GOT 0x84A0F0]; ldr w,[x10,x13]. */
    g_scriptSpace = *(uintptr_t*)(pGame + 0x84A0F0);
    logger->Info("DBG globals space=0x%lX", (unsigned long)g_scriptSpace);

    if(g_bHasBought && cfgUnlockAll && cfgUnlockAll->GetBool())
    {
        memset(g_bHasBought, 1, kBoughtSize);
        logger->Info("FreeClothes v1.3: all %d ownership flags unlocked", kBoughtSize);
    }

    logger->Info("FreeClothes v1.3: UnlockAll=%s FreePrice=%s AllShops=%s",
                 cfgUnlockAll->GetBool() ? "ON" : "OFF",
                 cfgFreePrice->GetBool() ? "ON" : "OFF",
                 cfgAllShops->GetBool()  ? "ON" : "OFF");
}