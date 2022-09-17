#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_image.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shaderReader.hpp"
#include "textureLoader.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const float SCR_WIDTH = 800;
const float SCR_HEIGHT = 600;

int windowWid = 800;
int windowHei = 600;

#define M 5

bool debugReportsActive = false;
bool updateProjection = false;

class Page;

struct Vector2i {
    int x = 0;
    int y = 0;
};

struct SplitData {
    bool splitted;
    bool full;
    bool overflown;
    int midKey;
    int lastKey;
    Page* newPage;
};

class Page {
public:
    int t = M;
    int n = 0;
    int keys[M];
    Page* links[M+1] = {};
    bool isLeaf = true;
    bool isRoot = false;
    Page* sibling = nullptr;

    Vector2i pos;

    void updatePos(Vector2i ref) {
        pos.x = ref.x;
        pos.y = ref.y;

        for (int i=0; i<=n; i++) {
            // printf("DRAW LINKS\n");
            if (links[i] != nullptr) {
                Vector2i newPos;
                newPos.x = pos.x;
                newPos.y = pos.y;
                newPos.y -= 100;
                newPos.x = i*t*50 + 50;
                links[i]->updatePos(newPos);
            }
        }
    }

    Vector2i getPos() {
        return pos;
    }

    void countLeaves(int& count) {
        if (isLeaf) {
            count += n+1;
            return;
        }
        else {
            for (int i=0; i<=n; i++) {
                if (links[i] != nullptr) {
                    links[i]->countLeaves(count);
                }
            }
        }
    }

    void alignLeavesPosition(int& leafIndex, int anchorPosition) {
        if (isLeaf) {
            pos.x = anchorPosition + leafIndex;
            leafIndex += (n+1)*50;
            return;
        }
        else {
            for (int i=0; i<=n; i++) {
                links[i]->alignLeavesPosition(leafIndex, anchorPosition);
            }
        }
    }

    Vector2i getLeftmostPosition() {
        if (isLeaf) {
            return pos;
        }
        else {
            return links[0]->getLeftmostPosition();
        }
    }

    Vector2i getRightmostPosition() {
        if (isLeaf) {
            Vector2i rightPos = pos;
            // rightPos.x += 50*5;
            return rightPos;
        }
        else {
            for (int i=n; i>=0; i--) {
                if (links[i] != nullptr) {
                    return links[i]->getRightmostPosition();
                }
            }
        }
        return pos;
    }

    void alignParents(int anchorPosition) {
        if (isLeaf) {
            return;
        }
        Vector2i left = getLeftmostPosition();
        Vector2i right = getRightmostPosition();
        if (isRoot) {
            // printf("left leaf pos: %d\n", left.x);
            // printf("right leaf pos: %d\n", right.x);
        }
        pos.x = left.x + ((right.x - left.x)/2);
        if (isRoot) {
            // printf("pos: %d\n", pos.x);
        }

        for (int i=0; i<=n; i++) {
            links[i]->alignParents(anchorPosition);
        }
    }

    void updateHeight(int base) {
        pos.y = base;
        for (int i=0; i<=n; i++) {
            if (links[i] != nullptr) {
                links[i]->updateHeight(base-100);
            }
        }
    }

    void updatePositions(glm::mat4& projection) {
        // printf("start update\n");
        int leavesCellCount = 0;
        countLeaves(leavesCellCount);
        // printf("leaves counted\n");
        int totalCellLength = leavesCellCount * 50;
        int leafIndex = 0;
        alignLeavesPosition(leafIndex, -totalCellLength/2);
        // printf("leaves aligned\n");
        alignParents(-totalCellLength/2);
        // printf("parents aligned\n");
        updateHeight(0);
        // printf("height updated\n");
        // std::vector<std::pair<int, int>> leafs;
    }

    void draw(unsigned int tfmLoc) {
        glm::vec2 transform(pos.x, pos.y);
        for (int i=0; i<n; i++) {
            glUniform2fv(tfmLoc, 1, glm::value_ptr(transform));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            transform.x += 50;
        }

        if (!isLeaf) {
            for (int i=0; i<=n; i++) {
                links[i]->draw(tfmLoc);
            }
        }

    }

    void writeKeyNumbers(uint transformLoc, uint cropLoc, int wid, int hei, uint* fontIndices) {
        glm::vec2 transform(pos.x+4.0f, pos.y + 20);
        for (int i=0; i<n; i++) {
            std::string key = std::to_string(keys[i]);
            glm::vec2 fontTransform = transform;
            for (int i=0; i<key.length(); i++) {
                char cpos = key.c_str()[i];
                // glm::vec4 crop( (1.0f/110.0f)*(0), (1.0f/110.0f)*0, (1.0f/110.0f), (1.0f/110.0f));
                // printf("cpos: %d\n", cpos);
                glm::vec4 crop( (1.0f/16.0f)*(cpos%16), (1.0f/8.0f)*(cpos/16), (1.0f/16.0f), (1.0f/8.0f));
                glUniform2fv(transformLoc, 1, glm::value_ptr(fontTransform));
                glUniform4fv(cropLoc, 1, glm::value_ptr(crop));
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
                fontTransform.x += 10;
            }
            // glUniform2fv(tfmLoc, 1, glm::value_ptr(transform));
            // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            transform.x += 50;
        }

        if (!isLeaf) {
            for (int i=0; i<=n; i++) {
                links[i]->writeKeyNumbers(transformLoc, cropLoc, wid, hei, fontIndices);
            }
        }
    }

    void drawLinks(uint shaderProgram) {
        if (isLeaf) {
            return;
        }
        uint pointsLoc = glGetUniformLocation(shaderProgram, "linePoints");
        for (int i=0; i<=n; i++) {
            // glm::vec3 points[] = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 100.0f, 0.0f), glm::vec3(100.0f, 100.0f, 0.0f)};
            float points[6] = {(float)(pos.x+(50*i)), (float)pos.y, 0, (float)links[i]->pos.x, (float)(links[i]->pos.y+50), 0.0f};
            glUniform3fv(pointsLoc, 2, points);
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, nullptr);

            links[i]->drawLinks(shaderProgram);
        }
    }

    Page() {

    }

    void report() {
        if (debugReportsActive) {
            printf("call from %s\n", isRoot ? "root" : "child");
            printf("call from %s leaf node\n", isLeaf ? "" : "non");
            printf("%d nodes in this page\n", n);
            printf("keys: ");
            if (n) {
                for (int i=0; i<n; i++) { printf("%d ", keys[i]);}
            }
            printf("\n");
        }
    }

    bool isFull() {
        return n == t-1;
    }

    bool isOverflown() {
        return n == t;
    }

    void shiftRight() {
        if (n == t) {
            printf("ATTEMPT TO SHIFT RIGHT ON A FULL PAGE\n");
        }
        links[n+1] = links[n];
        for (int i=n; i>0; i--) {
            keys[i] = keys[i-1];
            links[i] = links[i-1];
        }
        n++;
    }

    void shiftLeft() {
        for (int i=0; i<n-1; i++) {
            keys[i] = keys[i+1];
            links[i] = links[i+1];
        }
        links[n-1] = links[n];
        links[n] == nullptr;
        n--;
    }

    void setLeftmostKey(int substitute) {
        if (!isLeaf) {
            if (links[0]->isFull()) {
                // if is full, bring last key to this node
                shiftRight();
                keys[0] = links[0]->keys[n-1];
                links[0]->n--;
            }
            links[0]->setLeftmostKey(substitute);
        }
        else {
            printf("substitute set\n");
            // shifts and sets first key to parent caller value
            shiftRight();
            keys[0] = substitute;
        }
    }

    Page* getLeftmostPage() {
        if (!isLeaf) {
            return links[0]->getLeftmostPage();
        }
        return this;
    }

    Page* getRightmostPage() {
        if (!isLeaf) {
            return links[n]->getRightmostPage();
        }
        return this;
    }

    Page* split2by3(Page* p1, Page* p2, int kp, int& k1, int& k2) {
        int totalLength = p1->n + p2->n + 1; // calculate total keys
        Page* p3 = new Page(); // create new trird page
        int third = totalLength/3; // calculate 1/3 of total cell size, and n os the new pages
        int k1Pos = third; // gets position for key of parent 1
        int k2Pos = p2->n - third; // gets position for key of parent 2
        k1 = p1->keys[k1Pos]; // gets actual key
        k2 = p2->keys[k2Pos];
        int i;
        // copies second part of p2 to p3
        for (i = k2Pos+1; i < p2->n; i++) {
            p3->insert(p2->keys[i]);
        }
        p2->n = k2Pos; // "cuts" content copied from p2
        for (i = p1->n-1; i > k1Pos; i--) { // copies last elements of p1 to beginning of p2
            // p2->shiftRight();
            // p2->keys[0] = p1->keys[i];
            p2->insert(p1->keys[i]);
        }
        p2->insert(kp); // insert element that was parent of nodes
        p1->n = k1Pos;
        return p3;
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
        if (isOverflown() && isRoot) {
            // printf("root overflown\n");
            rootSplit();
        }
    }

    void split(int leftPage, int rightPage) {
        printf("splits of %d and %d\n", links[leftPage]->keys[0], links[rightPage]->keys[0]);
        int totalLength = links[leftPage]->n + links[rightPage]->n;
        int third = totalLength/3;
        printf("total length: %d\n", totalLength);
        printf("third: %d\n", third);
        int keyGroup[totalLength];
        int groupPos = 0;
        Page* linksGroup[totalLength+2] = {}; // each page has n+1 links, so 2 more spaces are needed
        int linkPos = 0;
        // copy all data to buffer, ordered
        for (int i=0; i<links[leftPage]->n; i++) {
            keyGroup[groupPos] = links[leftPage]->keys[i];
            printf("keys is %d\n", links[leftPage]->keys[i]);
            groupPos++;
        }
        // and copy the links to the pages
        for (int i=0; i<=links[leftPage]->n; i++) {
            linksGroup[linkPos] = links[leftPage]->links[i];
            linkPos++;
        }

        printf("Keys:\n");
        for (int i=0; i<groupPos; i++) {
            printf("%d, ", keyGroup[i]);
        }
        printf("\n");
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

        printf("Keys:\n");
        for (int i=0; i<groupPos; i++) {
            printf("%d, ", keyGroup[i]);
        }
        printf("\n");

        //also, copy the links of the right pages
        for (int i=0; i<=links[rightPage]->n; i++) {
            linksGroup[linkPos] = links[rightPage]->links[i];
            linkPos++;
        }
        printf(" linkPos: %d\n groupPos: %d\n totalLength: %d\n", linkPos, groupPos, totalLength);

        printf("Keys:\n");
        for (int i=0; i<groupPos; i++) {
            printf("%d, ", keyGroup[i]);
        }
        printf("\n");

        printf("shifting\n");
        // shifts parent elements to right by one position
        for (int i=n; i>=rightPage; i--) {
            keys[i] = keys[i-1];
            links[i+1] = links[i];
        }
        n++;
        printf("linking new page\n");
        // create new page at unnocupied link position
        links[rightPage+1] = new Page();
        // the new page leaf status is the same as the siblings
        links[rightPage+1]->isLeaf = links[rightPage]->isLeaf;
        // copy all data to designed points
        printf("copy first third\n");
        links[leftPage]->n = 0;
        for (int i=0; i<third; i++) {
            links[leftPage]->insertAtPage(keyGroup[i]);
        }
        printf("copy second third\n");
        links[rightPage]->n = 0;
        for (int i=third; i<third*2; i++) {
            links[rightPage]->insertAtPage(keyGroup[i]);
        }
        printf("copy last third\n");
        links[rightPage+1]->n = 0;
        for (int i=third*2; i<groupPos; i++) {
            links[rightPage+1]->insertAtPage(keyGroup[i]);
        }

        // now, return the child pages to apropriate position
        int linkIterator = 0;
        printf("copy first third links\n");
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

        printf("copy second third links\n");
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

        printf("copy last third links\n");
        links[rightPage+1]->links[0] = halfSplit; // no need to increase link iterator, because it is not at linkGroup array
        for (int i=1; i<=links[rightPage+1]->n; i++) {
            links[rightPage+1]->links[i] = linksGroup[linkIterator];
            linkIterator++;
        }
        // as the last of links, it doesn't need to be splitted

        printf("copy last third\n");
        links[rightPage+1]->n = 0;
        for (int i=third*2; i<groupPos; i++) {
            links[rightPage+1]->insertAtPage(keyGroup[i]);
        }

        keys[leftPage] = links[rightPage]->getLeftmostPage()->keys[0];
        keys[rightPage] = links[rightPage+1]->getLeftmostPage()->keys[0];
        printf("finishing splits\n");
    }

    void insert(int _key) {
        // initial state of tree is done here, until initial split
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
            printf("child node can't acomodate any more keys // performing rotation\n");

            // find case that conforms with child location
            if (i == 0) { // if this is the first child, it can only send nodes to right
                if (links[i+1]->isFull()) {
                    printf("PERFORM SPLIT HERE\n");
                    // links[i]->insertAtPage(_key);
                    split(i, i+1);
                }
                else {
                    // unified spill
                    printf("performs left-to-right spill\n");
                    // next opage NEEDS to be shifted right now
                    links[i+1]->shiftRight();
                    // in case the next node has children, the key must be set now, otherwise, it will be copied later
                    if (!links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                    // sets the new parent key copy as last of prev node
                    keys[i] = links[i]->keys[links[i]->n-1];
                    links[i+1]->links[0] = links[i]->links[links[i]->n];
                    // in case the page is a leaf, it needs a copy of parent key
                    if (links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                    links[i]->n--;
                    printf("spill is done\n");


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
                    printf("PERFORM SPLIT HERE\n");
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
                if (links[i+1]->n <= links[i-1]->n) {
                    // if right node has less keys than left node, it is better to pass keys to right
                    printf("MIDDLE-RIGHT PASS OF KEYS\n");
                    if (links[i+1]->isFull()) {
                        printf("PERFORM SPLIT HERE\n");
                        // links[i]->insertAtPage(_key);
                        split(i, i+1);
                    }
                    else {
                        // unified spill
                        printf("performs left-to-right spill\n");
                        // next opage NEEDS to be shifted right now
                        links[i+1]->shiftRight();
                        // in case the next node has children, the key must be set now, otherwise, it will be copied later
                        if (!links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                        // sets the new parent key copy as last of prev node
                        keys[i] = links[i]->keys[links[i]->n-1];
                        links[i+1]->links[0] = links[i]->links[links[i]->n];
                        // in case the page is a leaf, it needs a copy of parent key
                        if (links[i+1]->isLeaf) { links[i+1]->keys[0] = keys[i]; }
                        links[i]->n--;
                        printf("spill is done\n");
                    }
                }
                else {
                    // in this case, left has less keys, and receive the excess of keys
                    printf("MIDDLE-LEFT PASS OF KEYS\n");
                    if (links[i-1]->isFull()) {
                        printf("PERFORM SPLIT HERE\n");
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

    void rootSplit() {
        printf("Preparing root split\n");
        // self_draw(1);
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
        // preHalf->isRoot = false;
        // postHalf->isRoot = false
        isLeaf = false;
        if (preHalf->links[0] != nullptr) {preHalf->isLeaf = false;}
        if (postHalf->links[0] != nullptr) {postHalf->isLeaf = false;}

        if (preHalf->isLeaf) { preHalf->sibling = postHalf; }
        printf("Root split done\n");
    }

    void self_draw(int level) {
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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glm::mat4 projection;
    projection = glm::ortho(-SCR_WIDTH/2.0f, SCR_WIDTH/2.0f, -SCR_HEIGHT/2.0f, SCR_HEIGHT/2.0f, 0.0f, 100.0f);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    uint textureVS = createShader("shaders/texture.vert", GL_VERTEX_SHADER);
    uint textureFS = createShader("shaders/texture.frag", GL_FRAGMENT_SHADER);
    unsigned int textureSP = glCreateProgram();
    glAttachShader(textureSP, textureVS);
    glAttachShader(textureSP, textureFS);
    glLinkProgram(textureSP);
    glDeleteShader(textureVS);
    glDeleteShader(textureFS);

    uint textVS = createShader("shaders/text.vert", GL_VERTEX_SHADER);
    uint textFS = createShader("shaders/text.frag", GL_FRAGMENT_SHADER);
    unsigned int textSP = glCreateProgram();
    glAttachShader(textSP, textVS);
    glAttachShader(textSP, textFS);
    glLinkProgram(textSP);
    glDeleteShader(textVS);
    glDeleteShader(textFS);

    uint lineVS = createShader("shaders/lines.vert", GL_VERTEX_SHADER);
    uint lineFS = createShader("shaders/lines.frag", GL_FRAGMENT_SHADER);
    unsigned int lineSP = glCreateProgram();
    glAttachShader(lineSP, lineVS);
    glAttachShader(lineSP, lineFS);
    glLinkProgram(lineSP);
    glDeleteShader(lineVS);
    glDeleteShader(lineFS);

    float vertices[] = {
        // positions          // colors           // texture coords
         0.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   // top left
         50.0f,  0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // top right
         0.0f,   50.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
         50.0f,  50.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,    // bottom right

        // positions          // colors           // texture coords
         0.0f,   0.0f, 0.0f,         1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   // top left
         50.0f/4.0f,  0.0f, 0.0f,    0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // top right
         0.0f,   50.0f/4.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
         50.0f/4.0f,  50.0f/4.0f,    0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,    // bottom right

        // positions          // colors           // texture coords
         -800.0f/2.0f,   -600.0f/2.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,   // top left
         800.0f/2.0f,  -600.0f/2.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // top right
         -800.0f/2.0f,   600.0f/2.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
         800.0f/2.0f,  600.0f/2.0f, 0.0f,   1.0f, 1.0f, 0.0f,   1.0f, 0.0f,    // bottom right
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 2,  // first Triangle
        1, 2, 3   // second Triangle
    };

    unsigned int fontIndices[] = {  // note that we start from 0!
        4, 5, 6, // text quads
        5, 6, 7
    };

    unsigned int lineIndices[] = {  // note that we start from 0!
        4, 5, 6, // text quads
        5, 6, 7
    };

    unsigned int backgroundIndices[] = {
        8, 9, 10,
        9, 10, 11
    };

    unsigned int VBO, VAO, EBO, TEBO, BGEBO;

    uint cellTexture;
    glGenTextures(1, &cellTexture);
    glBindTexture(GL_TEXTURE_2D, cellTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char *data = stbi_load("cellTexture.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Failed to load texture" << std::endl; }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, cellTexture);

    uint fontTexture;
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int fontWidth, fontHeight, fontnrChannels;
    unsigned char *fontData = stbi_load("bitmapfont.jpg", &fontWidth, &fontHeight, &fontnrChannels, 0);
    if (fontData) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fontWidth, fontHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, fontData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Failed to load texture" << std::endl; }
    stbi_image_free(fontData);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    uint bgTexture;
    uint defaultBgTexture;
    int bgWidth, bgHeight, bgNrChannels;
    defaultBgTexture = loadTexture("bgImage.jpg", bgWidth, bgHeight, bgNrChannels);

    // load all background images
    std::vector<std::string> imagesPathes;
    std::string path = "backgrounds";
    if (std::filesystem::exists(path)) {
        for (const auto & entry : std::filesystem::directory_iterator(path)) {
            std::string entryPath = entry.path().string();
            if (entryPath.substr( entryPath.find_last_of(".") + 1) == "jpg") {
                imagesPathes.push_back(entryPath);
            }
        }
    }

    std::vector<uint> bgTextures;
    bgTextures.push_back(defaultBgTexture);
    for (int i=0; i<imagesPathes.size(); i++) {
        bgTextures.push_back( loadTexture(imagesPathes[i], bgWidth, bgHeight, bgNrChannels) );
    }
    bgTexture = defaultBgTexture;
    int currentTexture = 0;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glGenBuffers(1, &TEBO);
    glGenBuffers(1, &BGEBO);
    // glGenBuffers(1, &LEBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fontIndices), fontIndices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BGEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(backgroundIndices), backgroundIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 viewMatrix = glm::mat4(1.0f);
    // viewMatrix = glm::translate(viewMatrix, glm::vec3(50.0f, 0.0f, 0.0f));
    // viewMatrix = glm::rotate(viewMatrix, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));

    Page btree;
    btree.isRoot = true;
    btree.t = M;
    // change this if you want the tree to start with certain values
    // for (int i=5; i<=40; i+=5) {
    //     btree.insert(i);
    // }

    int nextInsert = 100;
    double insertCooldown = 0.2;
    double bgChangeCooldown = 0.2;
    double lastTimeRecord = glfwGetTime();
    double actualTimeRecord = glfwGetTime();
    double frameDifference;
    while (!glfwWindowShouldClose(window)) {
        projection = glm::ortho(-windowWid/2.0f, windowWid/2.0f, -windowHei/2.0f, windowHei/2.0f, 0.0f, 100.0f);
        actualTimeRecord = glfwGetTime();
        frameDifference = actualTimeRecord - lastTimeRecord;
        insertCooldown -= frameDifference;
        insertCooldown = (insertCooldown < 0) ? 0 : insertCooldown;
        bgChangeCooldown -= frameDifference;
        bgChangeCooldown = (bgChangeCooldown < 0) ? 0 : bgChangeCooldown;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(-5.0f, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(5.0f, 0.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, -5.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 5.0f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
            viewMatrix = glm::scale(viewMatrix, glm::vec3(0.95f, 0.95f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
            viewMatrix = glm::scale(viewMatrix, glm::vec3(1.05f, 1.05f, 0.0f));
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            viewMatrix = glm::mat4(1.0f);
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && insertCooldown <= 0) {
            btree.insert(++nextInsert);
            insertCooldown = 0.2;
        }
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && insertCooldown <= 0) {
            btree.insert(--nextInsert);
            insertCooldown = 0.2;
        }
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
            int userInsert = 0;
            printf("Digite numero: ");
            scanf("%d%*c", &userInsert);
            btree.insert(userInsert);
            nextInsert = userInsert;
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && bgChangeCooldown <= 0) {
            currentTexture = (currentTexture + 1) % bgTextures.size();
            bgTexture = bgTextures[currentTexture];
            bgChangeCooldown = 0.2;
        }
        if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && bgChangeCooldown <= 0) {
            currentTexture = (currentTexture - 1 + bgTextures.size()) % (bgTextures.size());
            bgTexture = bgTextures[currentTexture];
            bgChangeCooldown = 0.2;
        }
        lastTimeRecord = actualTimeRecord;
        // viewMatrix = glm::rotate(viewMatrix, 0.01f, glm::vec3(0.0f, 0.0f, 1.0f));
        // if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        //     printf("right\n");
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        btree.updatePositions(projection);

        glUseProgram(textureSP);

        glm::vec2 bgTransform = glm::vec2(0.0f, 0.0f);
        glm::vec2 transform = glm::vec2(0.0f, 0.0f);

        int transformLoc = glGetUniformLocation(textureSP, "transform");
        // std::cout << transformLoc << std::endl;
        int projectionLoc = glGetUniformLocation(textureSP, "projection");
        // std::cout << "cell projection " << projectionLoc << std::endl;
        if (projectionLoc < 0) {
            std::cout << "fail getting projection uniform location\n";
        }
        int viewLoc = glGetUniformLocation(textureSP, "view");

        // draw background
        glm::mat4 bgView(1.0f);
        glUniform2fv(transformLoc, 1, glm::value_ptr(transform));
        glm::mat4 bgProjection = glm::ortho(-SCR_WIDTH/2.0f, SCR_WIDTH/2.0f, -SCR_HEIGHT/2.0f, SCR_HEIGHT/2.0f, 0.0f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(bgProjection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(bgView));

        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, bgTexture);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(backgroundIndices), backgroundIndices, GL_STATIC_DRAW);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glUniform2fv(transformLoc, 1, glm::value_ptr(transform));

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_2D, cellTexture);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        btree.draw(transformLoc);

        glUseProgram(textSP);
        // glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fontTexture);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(fontIndices), fontIndices, GL_STATIC_DRAW);

        // std::cout << glGetError() << std::endl;
        transformLoc = glGetUniformLocation(textSP, "transform");
        projectionLoc = glGetUniformLocation(textSP, "projection");
        int cropLoc = glGetUniformLocation(textSP, "crop");
        viewLoc = glGetUniformLocation(textSP, "view");
        // std::cout << "transform " << transformLoc << std::endl;
        // std::cout << "projection " << projectionLoc << std::endl;
        // std::cout << "crop " << cropLoc << std::endl;
        // std::cout << "view " << viewLoc << std::endl;

        int errorIndex = 0;
        GLenum errorCode = glGetError();
        while (errorCode != GL_NO_ERROR) {
            std::cout << errorIndex << ": " << errorCode << std::endl;
            errorIndex++;
            errorCode = glGetError();
        }


        glm::mat4 fontViewMatrix = viewMatrix;
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        btree.writeKeyNumbers(transformLoc, cropLoc, fontWidth, fontHeight, fontIndices);
        // int errorIndex = 0;
        // GLenum errorCode = glGetError();
        // while (errorCode != GL_NO_ERROR) {
        //     std::cout << errorIndex << ": " << std::hex << errorCode << std::endl;
        //     errorIndex++;
        //     errorCode = glGetError();
        // }

        glUseProgram(lineSP);
        // glBindTexture(GL_TEXTURE_2D, cellTexture);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        projectionLoc = glGetUniformLocation(lineSP, "projection");
        viewLoc = glGetUniformLocation(lineSP, "view");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        btree.drawLinks(lineSP);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    windowWid = width;
    windowHei = height;
}
