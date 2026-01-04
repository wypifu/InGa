#include <InGa/core/container.h>

/*
void TestSTL() {
    printf("\n--- Test 1 : Utilisation normale (Pas de leak) ---\n");
    {
        Inga::Vector<Inga::String> v;
        v.push_back("Premier element");
        v.push_back("Deuxieme element avec une chaine assez longue pour eviter le SSO");
        // En sortant du scope, les destructeurs de String et Vector 
        // appellent deallocate(), donc ton Inga::Allocator::free().
    }
    printf("Succes : Scope termine.\n");

    printf("\n--- Test 2 : Leak d'un Vector dynamique ---\n");
    // On alloue le vecteur lui-meme avec INGA_NEW pour avoir le file/line
    // mais sa memoire INTERNE est geree par IngaAllocator (STL_Internal)
    auto* leakedVec = INGA_NEW Inga::Vector<int>();
    leakedVec->push_back(1);
    leakedVec->push_back(2);
    leakedVec->push_back(3);
    
    // Volontairement, on ne fait pas : delete leakedVec;
    printf("Leak genere : Un Vector de 3 elements n'a pas ete libere.\n");
}
*/

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
// 1. Initialisation avec tes paramètres par défaut
    // Rappel : start(U32 groupCount, U64 defaultPageSize)
    if (!Inga::Allocator::start(1, 1024 * 1024)) // 1 groupe de 1 Mo
    {
        printf("Erreur : Impossible de demarrer l'allocateur.\n");
        return -1;
    }

    float * p = INGA_NEW float[10];

    Inga::Allocator::stop(); 

    // On ne devrait jamais atteindre cette ligne si le crash fonctionne
    printf("Si vous lisez ceci, l'allocateur n'a pas crashe... ce n'est pas bon !\n");
    
    return 0;
}
