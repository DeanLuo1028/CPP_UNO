#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <map>
#include <ctime>
#include <memory>

using str = std::string;
// ---------------------------------------------------------
// 定義常數與枚舉
// ---------------------------------------------------------
class Color {
public:
    enum Value { RED, YELLOW, GREEN, BLUE, SPECIAL };
    
    static const std::vector<str> names;
    static const std::vector<str> strings;
private:
    Value value;

public:
    Color(Value v) : value(v) {}
    Color(const str &s) {
        auto it = std::find(strings.begin(), strings.end(), s);
        if(it == strings.end()) {
            throw std::out_of_range("輸入的字串找不到對應的顏色！");
        }
        value = static_cast<Value>(std::distance(strings.begin(), it));
    }

    str getName() const { return names[value]; }
    str getStr() const { return strings[value]; }

    bool operator==(const Color &other) const { return value == other.value; }
    // Value 本質上是 int ，直接傳值比 const Value& 快
    bool operator==(Value other) const { return value == other; }
    // Value 本質上是 int ，直接傳值比 const Value& 快
    bool operator!=(const Color &other) const { return value != other.value; }
    bool operator!=(Value other) const { return value != other; }
    bool operator<(const Color &other) const { return value < other.value; }
};
const std::vector<str> Color::names = {"RED", "YELLOW", "GREEN", "BLUE", "SPECIAL"}; // 一定要與Color::Value一一對應
const std::vector<str> Color::strings = {"R", "Y", "G", "B", "Special"}; // 一定要與Color::Value一一對應

class Rank {
public:
    enum Value { _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, SKIP, REVERSE, DRAW_2 , WILD, WILD_DRAW_4};

    static const std::vector<str> strings;
private:
    Value value;
public:
    Rank(Value v) : value(v) {}
    Rank(const str &s) {
        auto it = std::find(strings.begin(), strings.end(), s);
        if(it == strings.end()) {
            throw std::out_of_range("輸入的字串找不到對應的點數！");
        }
        value = static_cast<Value>(std::distance(strings.begin(), it));
    }

    str getStr() const { return strings[value]; }

    bool operator==(const Rank &other) const { return value == other.value; }
    bool operator==(Value other) const { return value == other; }
};
const std::vector<str> Rank::strings = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "Skip", "Reverse", "+2", "Wild", "Wild +4"}; // 一定要與Rank::Value一一對應

const std::vector<str> PLAYERS_NAME = {"Anna","Bob","Charlotte","Danny","Emily","Frank","Grace","Henry","Isabella","Jessica"}; // // 一定要與Rank::Value一一對應
const int CLOCKWISE = 1;
const int COUNTERCLOCKWISE = -1;

// 輔助函式：去除字串前後空白
// 在遊戲輸入時，玩家可能會輸入多餘的空白字元。
// 這個函式會移除字串開頭與結尾的空白，以便後續判斷輸入內容。
inline void trim(str &s) {
    if (s.empty()) return;
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
}


// ---------------------------------------------------------
// Class Card
// ---------------------------------------------------------
class Card {
public:
    Color color;
    Rank rank;

    Card(Color c, Rank r) : color(c), rank(r) {}

    // 將卡片內容格式化為可輸出字串，例如 "R 5" 或 "special +4"。
    str toString() const noexcept {
        return color.getStr() + " " + rank.getStr();
    }

    // 判斷這張牌是否符合出牌規則：
    // 1) 顏色相同
    // 2) 點數/功能相同
    // 3) 或者是特殊牌 (wild/+4) 可以搭配任何牌
    bool has_compliance_rules(const Card& discard_last_card) const noexcept {
        return this->color == discard_last_card.color ||
               this->rank == discard_last_card.rank ||
               this->color == Color::SPECIAL;
    }

    // 比較兩張牌是否完全相同 (顏色 + 點數/功能)。
    bool operator==(const Card &other) const noexcept {
        return color == other.color && rank == other.rank;
    }
};

std::ostream& operator<<(std::ostream& os, const Card& card) {
    os << card.toString();
    return os;
}

// ---------------------------------------------------------
// Class Deck
// ---------------------------------------------------------
class Deck {
public:
    std::vector<Card> cards;
    std::vector<Card> discard;
    std::mt19937 rng;

    // 建構子：初始化牌組 (108張)，並自動洗牌
    // - 0: 1張
    // - 1~9、Skip、Reverse、+2: 每種顏色各2張
    // - wild、+4: 各4張
    Deck() {
        rng.seed(static_cast<unsigned>(std::time(nullptr)));
        
        // 4種顏色
        for (std::size_t i=0; i<Color::strings.size(); ++i) { // 比 for (const str& color_str : Color::strings) {
            Color c{static_cast<Color::Value>(i)}; // Color c{color_str}; 更快，因為不用呼叫Color(const str &s)
            if (c == Color::SPECIAL) continue;
            cards.emplace_back(c, Rank::_0); // 每種顏色各有1張0
            // 數字1~9和"Skip", "Reverse", "+2"都各有兩張
            // 13和14是特殊牌的
            for (int j=1; j<13; ++j) { // 比 for (const str& rank_str : Rank::strings) {
                Rank r{static_cast<Rank::Value>(j)}; // Rank r{rank_str}; 更快，因為不用呼叫Rank(const str &s)
                cards.emplace_back(c, r);
                cards.emplace_back(c, r);
            }
        }
        // 特殊牌: wild 和 +4 各有4張
        for (int i = 0; i < 4; ++i) {
            cards.emplace_back(Color::SPECIAL, Rank::WILD);
            cards.emplace_back(Color::SPECIAL, Rank::WILD_DRAW_4);
        }

        shuffle();
    }

    // 將牌庫內的牌隨機重新排列。
    // 這在遊戲開始時，以及需要補牌時使用。
    inline void shuffle() {
        std::shuffle(cards.begin(), cards.end(), rng);
        std::cout << "洗牌完成!" << std::endl;
    }

    // 從牌堆中抽取一張牌。如果牌堆為空，則從棄牌堆補充牌堆。
    inline Card draw() {
        if (cards.empty()) {
            std::cout << "牌已經沒了！" << std::endl;
            replenish();
        }
        Card c = std::move(cards.back()); // 把牌組最後一張移動出來
        cards.pop_back(); // cards 減少一張
        return c;
    }

    // 當牌庫用盡時，從棄牌堆補牌並重新洗牌。
    // 會保留棄牌堆最上方的一張牌作為新的棄牌堆頂牌。
    // wild/+4 在棄牌後需還原為 SPECIAL 顏色，以便新一輪出牌可以選色。
    void replenish() {
        if (discard.empty()) {
            throw std::runtime_error("錯誤：棄牌堆為空，無法補牌！");
        }
        Card discard_last_card = std::move(discard.back());
        discard.pop_back();

        for (Card& card : discard) {
            if (card.rank == Rank::WILD || card.rank == Rank::WILD_DRAW_4) {
                card.color = Color::SPECIAL;
            }
        }
        
        cards = std::move(discard); // 將棄牌堆做為新的牌組
        discard.clear(); 
        discard.push_back(std::move(discard_last_card));
        shuffle();
    }

    void setFirstCard() {
        while (true) {
            Card card = std::move(draw());
            
            if (card.color == Color::SPECIAL || card.rank == Rank::DRAW_2 || 
                card.rank == Rank::SKIP || card.rank == Rank::REVERSE) { // 不能當底牌的牌
                cards.insert(cards.begin(), card); // 插回最底部
            } else { // card 可以當底牌
                discard.push_back(card);
                std::cout << "底牌是 " << card << std::endl;
                break;
            }
        }
    }
};

// ---------------------------------------------------------
// Class Player (Abstract Base Class)
// ---------------------------------------------------------
// 代表一名 UNO 玩家，包含共用的手牌與出牌邏輯。
// HumanPlayer 與 RobotPlayer 會繼承此類別並實作具體的出牌策略。
class Player {
public:
    str name;
    std::vector<Card> hand;

    Player(str n) : name(std::move(n)) {}
    
    // 虛擬解構子：確保多型時，資源能被正確釋放
    virtual ~Player() = default;

    // 純虛擬函式：強迫子類別實作
    virtual void deal(int, Deck&) = 0; 
    virtual bool play(Deck&) = 0;      
    virtual Color convert_color() = 0; 

    // 判斷玩家是否已出完手牌
    // 若手牌數為 0，即視為獲勝。
    bool win() const noexcept {
        if (hand.empty()) {
            std::cout << name << "獲勝了！" << std::endl;
            return true;
        }
        return false;
    }

    // 顯示玩家目前手牌，並標示每張牌的編號（方便玩家選擇出牌）。
    void display_hand() const {
        std::cout << name << "的手牌:" << std::endl;
        for (std::size_t i = 0; i < hand.size(); ++i) {
            std::cout << "第" << (i + 1) << "張:" << hand[i] << " ";
        }
        std::cout << std::endl;
    }

    // 執行玩家這一輪的動作：出牌或抽牌。
    // 回傳值表示動作結果，用於後續處理狀態（例如 Skip、Reverse、+2、+4、win）。
    str oneRound(Deck& deck) {
        if (deck.discard.empty()) {
            throw std::runtime_error("錯誤：棄牌堆為空，遊戲狀態異常！");
        }
        Card& top_card = deck.discard.back();
        std::cout << "現在牌堆最上方的牌: " << top_card << std::endl;

        // 是否不須抽牌
        bool played = this->play(deck); // 動態綁定

        if (!played) return "normal"; // 抽牌是普通情況
        if (win()) return "win";

        Card& played_card = deck.discard.back(); 

        if (played_card.color == Color::SPECIAL) {
            played_card.color = this->convert_color(); // 動態綁定
            if (played_card.rank == Rank::WILD) {
                return "normal";
            } else if (played_card.rank == Rank::WILD_DRAW_4) {
                return "+4";
            } else {
                throw std::runtime_error("錯誤!程式應該不會執行到這裡");
            }
        } else if (isdigit(played_card.rank.getStr()[0])) { // 一般數字牌
            return "normal";
        } else {
            return played_card.rank.getStr(); // Skip, Reverse, +2
        }
    }

    // 顯示玩家手牌數。當只剩一張時顯示 "UNO" 提示。
    void say_card_num() const noexcept {
        if (static_cast<int>(hand.size()) == 1) {
            std::cout << name << "說:UNO!" << std::endl;
        } else {
            std::cout << name << "剩" << hand.size() << "張" << std::endl;
        }
    }
};

// ---------------------------------------------------------
// Class RobotPlayer
// ---------------------------------------------------------
class RobotPlayer : public Player {
public:
    RobotPlayer(str n) : Player(std::move(n)) {}
    
    // 機器人收到指定張數的牌時，從牌庫抽牌並放到手牌。
    void deal(int cards_num, Deck& deck) override {
        for (int i = 0; i < cards_num; ++i) {
            hand.push_back(deck.draw());
        }
    }

    // 這個機器人出牌策略很簡單：
    // 1) 優先出與棄牌堆頂牌顏色或數字相符的普通牌（非 special）。
    // 2) 若沒有符合的普通牌，則出 special 牌 (wild/+4)。
    // 3) 無法出牌時，抽一張牌並結束回合。
    bool play(Deck& deck) override {
        Card& discard_last_card = deck.discard.back();

        // 優先出非特殊牌
        for (auto it = hand.begin(); it != hand.end(); ++it) {
            if (it->color != Color::SPECIAL && it->has_compliance_rules(discard_last_card)) {
                Card c = std::move(*it);
                hand.erase(it); // erase 使 it 失效，之後不應繼續迭代
                deck.discard.push_back(c);
                std::cout << name << "出了 " << c << std::endl;
                return true; // 不過因為這裡直接返回了所以實際上不會造成問題
            }
        }

        // 出特殊牌
        for (auto it = hand.begin(); it != hand.end(); ++it) {
            if (it->color == Color::SPECIAL) {
                Card c = std::move(*it);
                hand.erase(it); // erase 使 it 失效，之後不應繼續迭代
                deck.discard.push_back(c);
                std::cout << name << "出了 " << c << std::endl;
                return true; // 不過因為這裡直接返回了所以實際上不會造成問題
            }
        }

        std::cout << name << "沒牌可出，抽一張" << std::endl;
        deal(1, deck);
        return false;
    }

    // 當出現 wild/+4 需要選擇顏色時，此機器人會選擇手牌中數量最多的顏色。
    Color convert_color() override {
        std::map<Color, int> color_num;
        for (const auto& card : hand) {
            if (card.color != Color::SPECIAL) {
                color_num[card.color]++;
            }
        }
        
        int max_num = -1;
        Color max_color = Color::RED; 
        for (const auto &pair : color_num) {
            if (pair.second > max_num) {
                max_num = pair.second;
                max_color = pair.first;
            }
        }
        std::cout << name << "選擇了" << max_color.getName() << std::endl;
        return max_color;
    }
};

// ---------------------------------------------------------
// Class HumanPlayer
// ---------------------------------------------------------
class HumanPlayer : public Player {
public:
    HumanPlayer(str n) : Player(std::move(n)) {}

    // 人類玩家抽牌，並顯示抽到的牌。
    void deal(int cards_num, Deck& deck) override {
        for (int i = 0; i < cards_num; ++i) {
            Card c = deck.draw();
            hand.push_back(c);
            std::cout << name << "抽到了 " << c << std::endl;
        }
    }

    // 讓人類玩家選擇要出的牌，或選擇抽一張牌。
    // 這裡會持續詢問輸入直到拿到合法的選項。
    bool play(Deck& deck) override {
        Card& discard_last_card = deck.discard.back();
        std::cout << "以下是你的牌:" << std::endl;
        display_hand();

        while (true) {
            std::cout << name << "請問你要出第幾張牌?(如果不想出牌，請輸入0)" << std::endl;
            str s;
            std::getline(std::cin, s);
            trim(s);

            str s_lower = s;
            std::transform(s_lower.begin(), s_lower.end(), s_lower.begin(), ::tolower);
            
            if (s_lower == "quit") {
                std::cout << "遊戲結束!" << std::endl;
                exit(0);
            }

            bool is_digit = !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
            if (!is_digit) {
                std::cout << "請輸入一個有效的整數！" << std::endl;
                continue;
            }

            int user_input = std::stoi(s);
            if (user_input < 0 || user_input > static_cast<int>(hand.size())) {
                std::cout << "請輸入0~" << hand.size() << "之間的整數!" << std::endl;
                continue;
            }

            if (user_input == 0) {
                deal(1, deck);
                return false;
            } else {
                Card card = hand[user_input - 1]; // 減1是因為std::vector的索引從0開始
                if (!card.has_compliance_rules(discard_last_card)) {
                    std::cout << "請輸入顏色為" << discard_last_card.color.getStr()
                              << "或special的牌或者點數為" << discard_last_card.rank.getStr() << "的牌!" << std::endl;
                    continue;
                }

                hand.erase(hand.begin() + (user_input - 1));
                deck.discard.push_back(card);
                std::cout << name << "出了 " << card << std::endl;
                return true;
            }
        }
    }

    // 當出特殊牌 (wild / +4) 時，詢問人類玩家選擇新的顏色。
    // 會持續詢問直到輸入有效顏色代號為止。
    Color convert_color() override {
        while (true) {
            std::cout << name << "請選擇顏色:(請輸入RYGB其中之一)" << std::endl;
            str s;
            std::getline(std::cin, s);
            trim(s);
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            
            if (s != "R" && s != "Y" && s != "G" && s != "B") {
                std::cout << "請輸入有效的顏色代號!" << std::endl;
            } else {
                Color c(s);
                std::cout << name << "選擇了" << c.getName() << std::endl;
                return c;
            }
        }
    }
};

// ---------------------------------------------------------
// Class UNO
// ---------------------------------------------------------
// 管理整個遊戲流程：設定玩家、發牌、輪流出牌、處理特殊牌效果，以及判定遊戲結束。
class UNO {
public:
    int player_num;
    
    // 🔥 最佳實踐：使用 std::unique_ptr 自動管理指標生命週期
    std::vector<std::unique_ptr<Player>> players; 
    
    Deck deck;
    bool running = false;

    // 建構子：建立玩家 (1 個人類 + 其他電腦玩家)，並啟動遊戲循環。
    // 會在遊戲結束後詢問是否重新開始。
    UNO(int p_num, const str& human_name) : player_num(p_num) {
        // 🔥 最佳實踐：使用 std::make_unique 安全配置記憶體
        players.push_back(std::make_unique<HumanPlayer>(human_name));
        for (int i = 0; i < player_num - 1; ++i) {
            players.push_back(std::make_unique<RobotPlayer>(PLAYERS_NAME[i]));
        }
        
        do {
            setup();
            main_loop();
        } while (ask_restart());
    }
    
    // 不再需要手動寫 ~UNO() 解構子來 delete 指標了！

    // 設定遊戲初始狀態：清空手牌、發牌、並決定棄牌堆的第一張牌。
    // 若第一張牌是特殊牌（如 +2/Skip/Reverse 或 special），則放回牌庫重抽。
    void setup() {
        running = true;
        
        for (auto& p : players) {
            p->hand.clear();
            p->deal(7, deck);
        }

        deck.setFirstCard();
    }

    // 詢問玩家是否要重新開始遊戲。
    // 回傳 true 代表重新開始 (會再次執行 setup+main_loop)。
    bool ask_restart() {
        std::cout << "是否要重新開始遊戲？(y/n)" << std::endl;
        while (true) {
            str s;
            std::getline(std::cin, s);
            trim(s);
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);

            if (s == "y") return true;
            if (s == "n") {
                std::cout << "遊戲結束!" << std::endl;
                return false;
            }
            std::cout << "請輸入y或n!" << std::endl;
        }
    }

    // 主要遊戲迴圈，依序讓玩家出牌，並根據牌的效果變更順序、跳過玩家、強迫抽牌等。
    void main_loop() {
        std::cout << "遊戲開始!" << std::endl;
        int now_index = 0;
        int direction = CLOCKWISE;
        auto normalize_index = [&](int idx) {
            return (idx % player_num + player_num) % player_num;
        };

        while (running) {
            // 玩家行動後回傳狀態
            str state = players[now_index]->oneRound(deck);
            players[now_index]->say_card_num();

            if (state == "normal") {
                now_index = normalize_index(now_index + direction);
            } else if (state == "Skip") {
                // 跳過下一位玩家
                now_index = normalize_index(now_index + direction);
                std::cout << players[now_index]->name << "被跳過了" << std::endl;
                now_index = normalize_index(now_index + direction);
            } else if (state == "Reverse") {
                // 反轉出牌方向
                direction *= -1;
                std::cout << (direction == COUNTERCLOCKWISE ? "換成逆時針" : "換成順時針") << std::endl;
                now_index = normalize_index(now_index + direction);
            } else if (state == "+2") {
                // +2 牌會讓下一位玩家抽2張牌
                now_index = normalize_index(now_index + direction);
                std::cout << players[now_index]->name << "抽2張牌" << std::endl;
                players[now_index]->deal(2, deck);
            } else if (state == "+4") {
                // +4 牌會讓下一位玩家抽4張牌
                now_index = normalize_index(now_index + direction);
                std::cout << players[now_index]->name << "抽4張牌" << std::endl;
                players[now_index]->deal(4, deck);
            } else if (state == "win") {
                running = false;
                break;
            } else {
                throw std::runtime_error("錯誤!未知狀態: " + state);
            }

            std::cout << "下一個人是" << players[now_index]->name << "\n====================" << std::endl;
        }
    }
};

// ---------------------------------------------------------
// 主程式
// ---------------------------------------------------------
// 讀取玩家人數與玩家名稱，並啟動 UNO 遊戲。
int main() {
    int player_num;
    str human_name;
    std::cout << "歡迎來到UNO遊戲!" << std::endl;
    
    while (true) {
        std::cout << "請輸入玩家人數(2~10):" << std::endl;
        str s;
        std::getline(std::cin, s);
        trim(s);

        bool is_digit = !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
        if (!is_digit) {
            std::cout << "請輸入一個有效的整數！" << std::endl;
            continue;
        }

        player_num = std::stoi(s);
        if (player_num < 2 || player_num > 10) {
            std::cout << "請輸入2~10之間的整數!" << std::endl;
            continue;
        }
        break;
    }

    std::cout << "請輸入人類玩家的名字:" << std::endl;
    std::getline(std::cin, human_name);
    trim(human_name);
    if(human_name.empty()) human_name = "Player"; // 避免輸入全空白字元
    
    // 開始遊戲！因為使用了 unique_ptr，程式結束時不需要我們去清理記憶體，系統會自動回收。
    UNO game(player_num, human_name);
    
    return 0;
}