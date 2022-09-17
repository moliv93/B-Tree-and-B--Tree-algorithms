/* Author: Tiago Meneses de Oliveira */
#include <iostream>

#define M 5  // defines the degree of the tree (recommended above 7)

class Page {
public:
    int t = M;
    int n = 0;
    int keys[M];
    Page* links[M+1] = {};
    bool isLeaf = true;
    bool isRoot = false;

    Page() {

    }

    // full means the node has the maximum number of keys to be stable
    bool isFull() {
        return n == t-1;
    }

    // overflown means that the node is above the acceptable limit, and needs to be changed
    bool isOverflown() {
        return n == t;
    }

    // put all elements one position to the right, first position remais unchanged
    void shiftRight() {
        if (isFull()) {
            // in case that the node is full, only a message is sent
            printf("WARNING: ATTEMPT TO SHIFT RIGHT ON A FULL PAGE\n");
        }
        links[n+1] = links[n];
        for (int i=n; i>0; i--) {
            keys[i] = keys[i-1];
            links[i] = links[i-1];
        }
        if (!isOverflown()) {
            n++;
        }
    }

    // put all elements one position to the left, DELETES the first element
    void shiftLeft() {
        for (int i=0; i<n-1; i++) {
            keys[i] = keys[i+1];
            links[i] = links[i+1];
        }
        links[n-1] = links[n];
        links[n] = nullptr;
        n--;
    }

    // gets the leaf page at the bottom-left
    Page* getLeftmostPage() {
        if (!isLeaf) {
            return links[0]->getLeftmostPage();
        }
        return this;
    }

    // gets the leaf page at the bottom-right
    Page* getRightmostPage() {
        if (!isLeaf) {
            return links[n]->getRightmostPage();
        }
        else {
            return this;
        }
    }

    // insert at this page, and not at nodes below
    void insertAtPage(int _key) {
        int i=0;
        while (i < n && keys[i] < _key) { i++; }
        // insert node at this position
        for (int j=n; j>i; j--) {
            keys[j] = keys[j-1];
        }
        n++;
        keys[i] = _key;
    }

    void split(int leftPage, int rightPage) {
        int totalLength = links[leftPage]->n + links[rightPage]->n;
        int third = totalLength/3;
        int keyGroup[totalLength]; // this array allocates all keys participating on the split
        // in situations with secondary memory management, active copying is recommended
        int groupPos = 0;
        Page* linksGroup[totalLength+2] = {}; // each page has n+1 links, so 2 more spaces are needed
        int linkPos = 0;
        // copy all data to buffer, ordered
        for (int i=0; i<links[leftPage]->n; i++) {
            keyGroup[groupPos] = links[leftPage]->keys[i];
            groupPos++;
        }
        // and copy the links to the pages
        for (int i=0; i<=links[leftPage]->n; i++) {
            linksGroup[linkPos] = links[leftPage]->links[i];
            linkPos++;
        }
        // the key in the parent also counts in the array
        keyGroup[groupPos] = keys[leftPage];
        groupPos++;
        int copyStart=0;
        // but in case the split is done at leaf nodes, the parent element is already included, and the first key is skipped
        if (links[rightPage]->isLeaf) { copyStart++; }
        for (int i=copyStart; i<links[rightPage]->n; i++) {
            keyGroup[groupPos] = links[rightPage]->keys[i];
            groupPos++;
        }

        //also, copy the links of the right pages
        for (int i=0; i<=links[rightPage]->n; i++) {
            linksGroup[linkPos] = links[rightPage]->links[i];
            linkPos++;
        }

        // shifts parent elements to right by one position
        for (int i=n; i>=rightPage; i--) {
            keys[i] = keys[i-1];
            links[i+1] = links[i];
        }
        n++;
        // create new page at unnocupied link position
        links[rightPage+1] = new Page();
        // the new page leaf status is the same as the siblings
        links[rightPage+1]->isLeaf = links[rightPage]->isLeaf;
        // copy all data to designed points
        links[leftPage]->n = 0;
        for (int i=0; i<third; i++) {
            links[leftPage]->insertAtPage(keyGroup[i]);
        }
        // copy data to the second page
        links[rightPage]->n = 0;
        for (int i=third; i<third*2; i++) {
            links[rightPage]->insertAtPage(keyGroup[i]);
        }
        // copy data to the last page
        links[rightPage+1]->n = 0;
        for (int i=third*2; i<groupPos; i++) {
            links[rightPage+1]->insertAtPage(keyGroup[i]);
        }

        // now, return the child pages to apropriate position
        int linkIterator = 0;
        for (int i=0; i<=links[leftPage]->n; i++) {
            links[leftPage]->links[i] = linksGroup[linkIterator];
            linkIterator++;
        }
        // the last link of the page must be divided
        Page* halfSplit = nullptr;
        if (!links[leftPage]->isLeaf) {
            halfSplit = new Page();
            Page* lastLeaf = links[leftPage]->links[links[leftPage]->n];
            for (int i = lastLeaf->n/2; i < lastLeaf->n; i++) {
                halfSplit->insertAtPage(lastLeaf->keys[i]);
            }
            lastLeaf->n = lastLeaf->n/2;
        }

        // link children of the second page
        links[rightPage]->links[0] = halfSplit; // no need to increase link iterator, because it is not at linkGroup array
        for (int i=1; i<=links[rightPage]->n; i++) {
            links[rightPage]->links[i] = linksGroup[linkIterator];
            linkIterator++;
        }
        // the last link of the page must be divided
        halfSplit = nullptr;
        if (!links[rightPage]->isLeaf) {
            halfSplit = new Page();
            Page* lastLeaf = links[rightPage]->links[links[rightPage]->n];
            for (int i = lastLeaf->n/2; i < lastLeaf->n; i++) {
                halfSplit->insertAtPage(lastLeaf->keys[i]);
            }
            lastLeaf->n = lastLeaf->n/2;
        }

        // link last children
        links[rightPage+1]->links[0] = halfSplit; // no need to increase link iterator, because it is not at linkGroup array
        for (int i=1; i<=links[rightPage+1]->n; i++) {
            links[rightPage+1]->links[i] = linksGroup[linkIterator];
            linkIterator++;
        }
        // as the last of links, it doesn't need to be splitted

        // sets the new parent keys
        keys[leftPage] = links[rightPage]->getLeftmostPage()->keys[0];
        keys[rightPage] = links[rightPage+1]->getLeftmostPage()->keys[0];
        // the split is now done
    }

    // performs insertion of elements, and manage splits
    void insert(int _key) {
        if (isLeaf) {
            insertAtPage(_key);
            if (isRoot && isOverflown()) {
                rootSplit();
            }
            return;
        }

        int i=0;
        while (i < n && keys[i] < _key) { i++; }
        links[i]->insert(_key);
        if (links[i]->isOverflown()) {
            // find case that conforms with child location
            if (i == 0) { // if this is the first child, it can only send nodes to right
                if (links[i+1]->isFull()) {
                    split(i, i+1);
                }
                else { // left-to-right spill
                    // unified spill (single algoritm)
                    // next page NEEDS to be shifted right now
                    links[i+1]->shiftRight();
                    // in case the next node has children, the key must be set now, otherwise, it will be copied later
                    if (!links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                    // sets the new parent key copy as last of prev node
                    keys[i] = links[i]->keys[links[i]->n-1];
                    links[i+1]->links[0] = links[i]->links[links[i]->n];
                    // in case the page is a leaf, it needs a copy of parent key
                    if (links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                    links[i]->n--;


                    // separated logic for spill, might be easier to understand
                    // if (links[i]->isLeaf) {
                    //     if (links[i+1]->isLeaf) { links[i+1]->shiftRight(); }
                    //     keys[i] = links[i]->keys[links[i]->n-1];
                    //     links[i+1]->keys[0] = keys[i];
                    //     links[i]->n--;
                    // }
                    // else {
                    // // non-leaf
                    //     if (!links[i+1]->isLeaf) { links[i+1]->shiftRight(); }
                    //     links[i+1]->keys[0] = keys[i];
                    //     links[i+1]->links[0] = links[i]->links[links[i]->n];
                    //     keys[i] = links[i]->keys[links[i]->n-1];
                    //     links[i]->n--;
                    // }
                }
            }
            else if (i == n) { // if this is the last, it can only send nodes left
                if (links[i-1]->isFull()) {
                    split(i-1, i);
                }
                else {
                    // push first element to the left
                    // push key at parent to prev node
                    links[i-1]->insertAtPage(keys[i-1]);
                    // leftmost page gets "rotated" to left side
                    links[i-1]->links[links[i-1]->n] = links[i]->links[0];
                    // next, the order of shifting shall differ from leaf and non-leaf nodes
                    // possible substitution: create a function to find first non-equal key at this page
                    // decrease the first element of the page
                    if (links[i]->isLeaf) { links[i]->shiftLeft(); }
                    // scale the new first element to list of keys
                    keys[i-1] = links[i]->keys[0];
                    // decrease the first element of the page
                    if (!links[i]->isLeaf) { links[i]->shiftLeft(); }
                }
            }
            else { // if is in the middle, we can choose direction
                // each case follows the same logic of the two above
                if (links[i+1]->n <= links[i-1]->n) {
                    // if right node has less keys than left node, it is better to pass keys to right
                    if (links[i+1]->isFull()) {
                        split(i, i+1);
                    }
                    else {
                        // unified spill
                        // next page NEEDS to be shifted right now
                        links[i+1]->shiftRight();
                        // in case the next node has children, the key must be set now, otherwise, it will be copied later
                        if (!links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                        // sets the new parent key copy as last of prev node
                        keys[i] = links[i]->keys[links[i]->n-1];
                        links[i+1]->links[0] = links[i]->links[links[i]->n];
                        // in case the page is a leaf, it needs a copy of parent key
                        if (links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                        links[i]->n--;
                    }
                }
                else {
                    // in this case, left has less keys, and receive the excess of keys
                    if (links[i-1]->isFull()) {
                        split(i-1, i);
                    }
                    else {
                        // push first element to the left
                        // push key at parent to prev node
                        links[i-1]->insertAtPage(keys[i-1]);
                        // leftmost page gets "rotated" to left side
                        links[i-1]->links[links[i-1]->n] = links[i]->links[0];
                        // next, the order of shifting shall differ from leaf and non-leaf nodes
                        // possible substitution: create a function to find first non-equal key at this page
                        // decrease the first element of the page
                        if (links[i]->isLeaf) { links[i]->shiftLeft(); }
                        // scale the new first element to list of keys
                        keys[i-1] = links[i]->keys[0];
                        // decrease the first element of the page
                        if (!links[i]->isLeaf) { links[i]->shiftLeft(); }
                    }
                }
            }
        }
        // as this will not return anywhere, rootSplit has to be done here
        if (isRoot && isOverflown()) {
            rootSplit();
        }
    }

    // splits the root in half, creating 2 new nodes
    void rootSplit() {
        Page* preHalf = new Page();
        Page* postHalf = new Page();
        int halfPos = n/2;

        for (int i=0; i<halfPos; i++) {
            preHalf->keys[i] = keys[i];
            preHalf->links[i] = links[i];
        }
        preHalf->links[halfPos] = links[halfPos];
        preHalf->n = halfPos;

        // only copies mid element IF childs are leafs, else, mid element is already present at leaf level
        int copyPos = halfPos;
        if (links[0] != nullptr) { copyPos = halfPos + 1; }
        for (int i=copyPos; i<=n; i++) {
            postHalf->keys[i-(copyPos)] = keys[i];
            postHalf->links[i-(copyPos)] = links[i];
        }
        postHalf->links[n-copyPos] = links[n];
        postHalf->n = n-copyPos;

        keys[0] = keys[halfPos];
        links[0] = preHalf;
        links[1] = postHalf;
        n = 1;
        isLeaf = false;
        if (preHalf->links[0] != nullptr) {preHalf->isLeaf = false;}
        if (postHalf->links[0] != nullptr) {postHalf->isLeaf = false;}
    }

    bool search(int _key) {
        int i=0;
        while (i < n && keys[i] <= _key) {
            if (keys[i] == _key) return true;
            i++;
        }
        if (isLeaf)
            return false; // in case this is a leaf and the element is not found yet, there is nowhere to search for
        else
            return links[i]->search(_key);
    }

    // draw key array of each page per level
    void self_draw(int level) {
        for (int i=0; i<level*4; i++) { printf(" "); }
        printf("%d%s: [", level, isLeaf ? "yL" : "nL");
        printf("%c", links[0] == nullptr ? '-' : '+');
        for (int i=0; i<n; i++) {
            printf("%d%c ", keys[i], links[i+1] == nullptr ? '-' : '+');
        }
        printf("]\n");
        for (int i=0; i<=n; i++) {
            if (links[i] != nullptr) {
                links[i]->self_draw(level+1);
            }
        }
    }
};

int main(int argc, char *argv[]) {
    Page* btree = nullptr;
    // btree.isRoot = true;

    // change this if you want the tree to start with certain values
    // btree = new Page()
    // btree->isRoot = true;
    // for (int i=5; i<=200; i+=5) {
    //     btree.insert(i);
    //     btree.self_draw(1);
    //     btree.insert(400-i);
    //     btree.self_draw(1);
    // }

    int choice = 0;
    enum choices { CREATE = 1, INSERT, SEARCH, SHOW, END};
    while (choice != END) {
        printf("\nEscolha: \n");
        printf("%d: criar arvore\n", CREATE);
        printf("%d: inserir elemento\n", INSERT);
        printf("%d: procurar elemento\n", SEARCH);
        printf("%d: mostrar os dados\n", SHOW);
        printf("%d: finalizar\n", END);
        scanf("%d%*c", &choice);

        if (choice == CREATE) {
            btree = new Page();
            btree->isRoot = true;
        }
        if (choice == INSERT) {
            if (btree != nullptr) {
                int dataInsert;
                printf("Digite o valor a ser inserido: \n");
                scanf("%d%*c", &dataInsert);
                btree->insert(dataInsert);
            }
            else {
                printf("Crie uma arvore primeiro\n");
            }
        }
        if (choice == SEARCH) {
            if (btree != nullptr) {
                int dataSearch;
                printf("Digite o valor a ser pesquisado: \n");
                scanf("%d%*c", &dataSearch);
                if (btree->search(dataSearch))
                    printf("O valor %d se encontra na árvore\n", dataSearch);
                else
                    printf("O valor %d NAO se encontra na arvore\n", dataSearch);
            }
            else {
                printf("Crie uma arvore primeiro\n");
            }
        }
        if (choice == SHOW) {
            if (btree != nullptr) {
                btree->self_draw(1);
            }
            else {
                printf("Crie uma arvore primeiro\n");
            }
        }
        if (choice == END) {
            printf("finalizando...\n");
        }
    }
}
