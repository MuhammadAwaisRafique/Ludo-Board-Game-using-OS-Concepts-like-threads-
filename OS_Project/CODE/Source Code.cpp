// GROUP MEMBERS:
// 1. Azmar kashif (22i-2716-SE)
// 2. Awais Rafique (22i-2454-SE)
// 3. Hassaan (22i-2511-SE)

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QColor>
#include <QRectF>
#include <QPolygonF>
#include <QPainterPath>
#include <vector>
#include <iostream>
#include <map>
#include <QTime> // For QElapsedTimer
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QStack>
#include <QPropertyAnimation>
#include <QSemaphore>
#include <pthread.h>
#include <semaphore.h>
#include <thread>
#include <unistd.h>
#include <semaphore>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QAbstractAnimation>
#include <QParallelAnimationGroup>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QAbstractAnimation>
#include <QParallelAnimationGroup>
#include <QSpinBox>
#include <QPainter>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QDialog>
#include <QColorDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QStyle>
#include <QRadioButton>
#include <QButtonGroup>
using namespace std;
#define GRID_SIZE 15 // Total number of rows and columns
#define TILE_SIZE 40 // Size of each square tile
#define MAX_PLAYERS 4
#define MAX_TOKENS 4
bool canMoveToSafeZone = false;

typedef struct
{
    int row;
    int column;
    int hit_record;
    int player_id;
} parameters;

struct tokenThread
{
    int x, y;
    QColor color;
    bool inSafeZone;
    bool isHome;
    sem_t semaphore;
};

struct Player
{
    std::vector<tokenThread> tokens;
    int hit_record;
    int turns_without_six;
    bool is_active;
    pthread_t thread_id;
};
struct PlayerStats
{
    int tokenCount;
    int finishedTokens;
    int hits;
    bool isActive;
    QColor color;

    PlayerStats(int tokens = 4, QColor col = Qt::blue)
        : tokenCount(tokens), finishedTokens(0), hits(0), isActive(true), color(col) {}

    void reset()
    {
        finishedTokens = 0;
        hits = 0;
        isActive = true;
    }
};
struct PlayerStatsStyle
{
    QColor textColor = Qt::white;
    QColor backgroundColor = Qt::transparent;
    QColor borderColor = Qt::white;
    int fontSize = 12;
    int padding = 8;
    int borderRadius = 6;
    double opacity = 0.85;
    bool showBackground = true;
};

struct PlayerStatsPosition
{
    int x;
    int y;
    Qt::Alignment alignment;

    PlayerStatsPosition(int _x, int _y, Qt::Alignment _alignment)
        : x(_x), y(_y), alignment(_alignment) {}
};

struct Dice
{
    bool isRolling = false;
    int value = 1;           // Starting value of dice
    float rotation = 0;      // Rotation angle
    QElapsedTimer rollClock; // Timer to measure dice roll time
};

struct PathCoordinate
{
    int x, y;
    bool isColoredPath;
    int playerColor; // -1 for white path, 0-3 for colored paths
};

struct Token
{
    int x;
    int y;
    QColor color;
    bool isHome;
    bool inSafeZone;
    QString tokenType;

    Token() : x(0), y(0), color(Qt::white), isHome(true), inSafeZone(false), tokenType("Circle") {}
};

struct TurnState
{
    std::vector<Token> tokenPositions;
    int diceValue;
};

struct PlayerRank
{
    int playerId;
    int rank; // 1 for 1st, 2 for 2nd, etc.
    bool hasFinished;
};

std::vector<PlayerRank> playerRankings;
std::map<int, bool> finishedPlayers;
class LudoGame
{
private:
    static const int GRID_SIZE_MAX = 15;
    static const int MAX_TURNS_WITHOUT_SIX = 20;
    std::vector<Player> players;
    tokenThread grid[GRID_SIZE_MAX][GRID_SIZE_MAX];
    pthread_mutex_t board_mutex;
    pthread_cond_t turn_cond;
    int current_player;
    bool game_over;

    pthread_t hit_check_thread;
    pthread_t master_thread;

public:
    LudoGame() : current_player(0), game_over(false)
    {
        pthread_mutex_init(&board_mutex, NULL);
        pthread_cond_init(&turn_cond, NULL);
        initializePlayersThread();
        pthread_create(&hit_check_thread, NULL, hitCheckThreadFunctionThread, this);
        pthread_create(&master_thread, NULL, masterThreadFunction, this);
    }

    ~LudoGame()
    {
        pthread_mutex_destroy(&board_mutex);
        pthread_cond_destroy(&turn_cond);
        for (auto &player : players)
        {
            for (auto &token : player.tokens)
            {
                sem_destroy(&token.semaphore);
            }
        }
    }

    void initializePlayersThread()
    {
        std::cout << "Displaying grid" << std::endl;
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            std::cout << "Threads " << i << " initialized players " << i << std::endl;
            Player player;
            player.hit_record = 0;
            player.turns_without_six = 0;
            player.is_active = true;
            for (int j = 0; j < MAX_TOKENS; ++j)
            {
                std::cout << "Displaying tokens for player " << i << " Thread " << std::endl;
                tokenThread token;
                token.x = -1;
                token.y = -1;
                token.inSafeZone = false;
                token.isHome = true;
                sem_init(&token.semaphore, 0, 1);
                player.tokens.push_back(token);
            }
            players.push_back(player);
        }
        std::cout << "Players initialized." << std::endl;
    }

    void playTurnThread(int player_id, int dice_value)
    {
        pthread_mutex_lock(&board_mutex);
        while (current_player != player_id)
        {
            pthread_cond_wait(&turn_cond, &board_mutex);
        }

        Player &player = players[player_id];
        if (dice_value == 6)
        {
            player.turns_without_six = 0;
        }
        else
        {
            player.turns_without_six++;
        }

        current_player = (current_player + 1) % MAX_PLAYERS;
        pthread_cond_broadcast(&turn_cond);
        pthread_mutex_unlock(&board_mutex);
        std::cout << "Player " << player_id + 1 << " rolled a " << dice_value << "." << std::endl;
    }

    static void *hitCheckThreadFunctionThread(void *arg)
    {
        LudoGame *game = static_cast<LudoGame *>(arg);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, game->GRID_SIZE_MAX - 1);

        while (!game->game_over)
        {
            int row = dis(gen);
            int col = dis(gen);
            game->checkHitThread(row, col);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return NULL;
    }

    void checkHitThread(int row, int col)
    {
        pthread_mutex_lock(&board_mutex);

        tokenThread &token = grid[row][col];
        if (token.x != -1 && token.y != -1)
        {
            for (int i = 0; i < players.size(); ++i)
            {
                for (auto &other_token : players[i].tokens)
                {
                    if (other_token.x == row && other_token.y == col && &other_token != &token)
                    {
                        players[i].hit_record++;
                        other_token.x = -1;
                        other_token.y = -1;
                        other_token.inSafeZone = false;
                        other_token.isHome = true;

                        grid[row][col].x = -1;
                        grid[row][col].y = -1;
                        std::cout << "Hit detected at (" << row << ", " << col << "). Token returned to home." << std::endl;
                    }
                }
            }
        }

        pthread_mutex_unlock(&board_mutex);
    }

    static void *masterThreadFunction(void *arg)
    {
        LudoGame *game = static_cast<LudoGame *>(arg);
        while (!game->game_over)
        {
            game->checkPlayerStatus();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        return NULL;
    }

    void checkPlayerStatus()
    {
        pthread_mutex_lock(&board_mutex);
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (players[i].is_active)
            {
                if (players[i].turns_without_six >= MAX_TURNS_WITHOUT_SIX)
                {
                    players[i].is_active = false;
                    pthread_cancel(players[i].thread_id);
                    std::cout << "Player " << i + 1 << " kicked out due to 20 turns without a 6." << std::endl;
                }
                if (allTokensAreHome(i))
                {
                    announceWinner(i);
                    players[i].is_active = false;
                    pthread_cancel(players[i].thread_id);
                }
            }
        }
        checkGameOver();
        pthread_mutex_unlock(&board_mutex);
    }

    bool allTokensAreHome(int player_id)
    {
        for (const auto &token : players[player_id].tokens)
        {
            if (!token.isHome)
            {
                return false;
            }
        }
        return true;
    }

    void announceWinner(int player_id)
    {
        pthread_mutex_lock(&board_mutex);
        std::cout << "Player " << player_id + 1 << " has won the game!" << std::endl;
        game_over = true;
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (i != player_id && players[i].is_active)
            {
                players[i].is_active = false;
                pthread_cancel(players[i].thread_id);
            }
        }
        pthread_mutex_unlock(&board_mutex);
    }

    void checkGameOver()
    {
        int active_players = std::count_if(players.begin(), players.end(),
                                           [](const Player &p)
                                           { return p.is_active; });
        if (active_players <= 1)
        {
            game_over = true;
            std::cout << "Game over. Only one player left." << std::endl;
        }
    }

    void displayGrid()
    {
        pthread_mutex_lock(&board_mutex);
        std::cout << "Grid Display:" << std::endl;
        for (int i = 0; i < GRID_SIZE_MAX; ++i)
        {
            for (int j = 0; j < GRID_SIZE_MAX; ++j)
            {
                std::cout << "Displaying grid" << endl;
            }
            std::cout << std::endl;
        }
        pthread_mutex_unlock(&board_mutex);
    }

    void moveToken(int player_id, int token_id, int steps)
    {
        pthread_mutex_lock(&board_mutex);
        tokenThread &token = players[player_id].tokens[token_id];
        if (token.isHome)
        {
            if (steps == 6)
            {
                token.x = 0;
                token.y = 0;
                token.isHome = false;
                std::cout << "Token " << token_id + 1 << " of Player " << player_id + 1 << " entered the board." << std::endl;
            }
        }
        else
        {
            // Move token logic here
            std::cout << "Token " << token_id + 1 << " of Player " << player_id + 1 << " moved " << steps << " steps." << std::endl;
        }
        pthread_mutex_unlock(&board_mutex);
    }
};
class TokenDrawer
{
public:
    static void drawToken(QPainter &painter, const Token &token, int tileSize);

private:
    static void drawStarToken(QPainter &painter, const QRectF &rect);
    static void drawLightningToken(QPainter &painter, const QRectF &rect);
    static void drawDiamondToken(QPainter &painter, const QRectF &rect);
    static void drawCircleToken(QPainter &painter, const QRectF &rect);
};
void TokenDrawer::drawToken(QPainter &painter, const Token &token, int tileSize)
{
    QRectF tokenRect(token.x * tileSize + 5,
                     token.y * tileSize + 5,
                     tileSize - 10,
                     tileSize - 10);

    painter.save();
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(token.color);

    if (token.tokenType == "Star")
    {
        drawStarToken(painter, tokenRect);
    }
    else if (token.tokenType == "Lightning")
    {
        drawLightningToken(painter, tokenRect);
    }
    else if (token.tokenType == "Diamond")
    {
        drawDiamondToken(painter, tokenRect);
    }
    else
    {
        drawCircleToken(painter, tokenRect);
    }

    painter.restore();
}

void TokenDrawer::drawStarToken(QPainter &painter, const QRectF &rect)
{
    const int points = 5;
    const double outerRadius = rect.width() / 2;
    const double innerRadius = rect.width() / 4;
    QPolygonF star;
    QPointF center = rect.center();

    for (int i = 0; i < points * 2; ++i)
    {
        double radius = (i % 2 == 0) ? outerRadius : innerRadius;
        double angle = i * M_PI / points - M_PI / 2;
        star << QPointF(center.x() + radius * cos(angle),
                        center.y() + radius * sin(angle));
    }

    painter.drawPolygon(star);
}

void TokenDrawer::drawLightningToken(QPainter &painter, const QRectF &rect)
{
    QPolygonF lightning;
    QPointF center = rect.center();
    double w = rect.width();
    double h = rect.height();

    lightning << QPointF(center.x() - w / 4, rect.top())
              << QPointF(center.x() + w / 4, center.y() - h / 4)
              << QPointF(center.x(), center.y())
              << QPointF(center.x() - w / 4, center.y() + h / 4)
              << QPointF(center.x() + w / 4, rect.bottom());

    painter.drawPolygon(lightning);
}

void TokenDrawer::drawDiamondToken(QPainter &painter, const QRectF &rect)
{
    QPolygonF diamond;
    QPointF center = rect.center();

    diamond << QPointF(center.x(), rect.top())
            << QPointF(rect.right(), center.y())
            << QPointF(center.x(), rect.bottom())
            << QPointF(rect.left(), center.y());

    painter.drawPolygon(diamond);
}

void TokenDrawer::drawCircleToken(QPainter &painter, const QRectF &rect)
{
    painter.drawEllipse(rect);
}
class TokenSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    TokenSelectionDialog(bool is2PMode, QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(is2PMode ? "2 Player Setup" : "4 Player Setup");
        setModal(true);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setSpacing(20);
        layout->setContentsMargins(30, 30, 30, 30);

        // Title
        QLabel *titleLabel = new QLabel("Token Setup", this);
        titleLabel->setStyleSheet(
            "font-size: 24px;"
            "color: #FFD700;"
            "padding: 10px;");
        layout->addWidget(titleLabel, 0, Qt::AlignCenter);

        // Token quantity selection
        QLabel *tokenLabel = new QLabel("Select number of tokens per player:", this);
        tokenLabel->setStyleSheet("color: white; font-size: 16px;");
        layout->addWidget(tokenLabel);

        m_tokenSpinBox = new QSpinBox(this);
        m_tokenSpinBox->setRange(1, 4);
        m_tokenSpinBox->setValue(4);
        m_tokenSpinBox->setStyleSheet(getSpinBoxStyle());
        layout->addWidget(m_tokenSpinBox);

        // Token type selection
        QLabel *typeLabel = new QLabel("Select token type:", this);
        typeLabel->setStyleSheet("color: white; font-size: 16px;");
        layout->addWidget(typeLabel);

        m_tokenTypeComboBox = new QComboBox(this);
        m_tokenTypeComboBox->addItems({"Circle", "Star", "Lightning", "Diamond"});
        m_tokenTypeComboBox->setStyleSheet(getComboBoxStyle());
        layout->addWidget(m_tokenTypeComboBox);

        if (is2PMode)
        {
            setupPlayerColors(layout);
        }

        // Start Game button
        QPushButton *okButton = new QPushButton("Start Game", this);
        okButton->setStyleSheet(getButtonStyle());
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(okButton);

        // Set dialog styling
        setStyleSheet("QDialog { background-color: #2A0944; border-radius: 10px; }");
    }

    int getTokenCount() const { return m_tokenSpinBox->value(); }
    QString getTokenType() const { return m_tokenTypeComboBox->currentText(); }
    QColor getPlayer1Color() const { return m_player1Color ? getColorFromIndex(m_player1Color->currentIndex()) : Qt::blue; }
    QColor getPlayer2Color() const { return m_player2Color ? getColorFromIndex(m_player2Color->currentIndex()) : Qt::red; }

private:
    void setupPlayerColors(QVBoxLayout *layout)
    {
        // Player 1 color selection
        QLabel *p1Label = new QLabel("Player 1 Color:", this);
        p1Label->setStyleSheet("color: white; font-size: 16px;");
        layout->addWidget(p1Label);

        m_player1Color = new QComboBox(this);
        setupColorComboBox(m_player1Color);
        layout->addWidget(m_player1Color);

        // Player 2 color selection
        QLabel *p2Label = new QLabel("Player 2 Color:", this);
        p2Label->setStyleSheet("color: white; font-size: 16px;");
        layout->addWidget(p2Label);

        m_player2Color = new QComboBox(this);
        setupColorComboBox(m_player2Color);
        m_player2Color->setCurrentIndex(1);
        layout->addWidget(m_player2Color);

        // Prevent same color selection
        connect(m_player1Color, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index)
                {
                        if (m_player2Color->currentIndex() == index) {
                            m_player2Color->setCurrentIndex((index + 1) % 4);
                        } });

        connect(m_player2Color, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index)
                {
                        if (m_player1Color->currentIndex() == index) {
                            m_player1Color->setCurrentIndex((index + 1) % 4);
                        } });
    }

    void setupColorComboBox(QComboBox *box)
    {
        box->addItem("Blue", QColor(Qt::blue));
        box->addItem("Red", QColor(Qt::red));
        box->addItem("Yellow", QColor(Qt::yellow));
        box->addItem("Green", QColor(Qt::green));
        box->setStyleSheet(getComboBoxStyle());
    }

    QString getSpinBoxStyle() const
    {
        return "QSpinBox {"
               "    background-color: #4A235A;"
               "    color: white;"
               "    border: 2px solid #6C3483;"
               "    border-radius: 8px;"
               "    padding: 8px;"
               "    font-size: 16px;"
               "}";
    }

    QString getComboBoxStyle() const
    {
        return "QComboBox {"
               "    background-color: #4A235A;"
               "    color: white;"
               "    border: 2px solid #6C3483;"
               "    border-radius: 8px;"
               "    padding: 8px;"
               "    font-size: 16px;"
               "}"
               "QComboBox QAbstractItemView {"
               "    background-color: #4A235A;"
               "    color: white;"
               "    selection-background-color: #6C3483;"
               "    border: 1px solid #6C3483;"
               "}";
    }

    QString getButtonStyle() const
    {
        return "QPushButton {"
               "    background-color: #27AE60;"
               "    color: white;"
               "    border: none;"
               "    border-radius: 8px;"
               "    font-size: 18px;"
               "    font-weight: bold;"
               "    padding: 15px;"
               "    margin-top: 20px;"
               "}"
               "QPushButton:hover {"
               "    background-color: #2ECC71;"
               "}"
               "QPushButton:pressed {"
               "    background-color: #219A52;"
               "}";
    }

    QColor getColorFromIndex(int index) const
    {
        switch (index)
        {
        case 0:
            return Qt::blue;
        case 1:
            return Qt::red;
        case 2:
            return Qt::yellow;
        case 3:
            return Qt::green;
        default:
            return Qt::blue;
        }
    }

    QSpinBox *m_tokenSpinBox;
    QComboBox *m_tokenTypeComboBox;
    QComboBox *m_player1Color;
    QComboBox *m_player2Color;
};

class PlayerInitializer : public QThread
{
    Q_OBJECT
public:
    PlayerInitializer(std::map<int, std::vector<Token>> &playerTokens, int playerId,
                      int tokenCount, QString tokenType, QColor playerColor)
        : m_playerTokens(playerTokens), m_playerId(playerId),
          m_tokensPerPlayer(tokenCount), m_tokenType(tokenType),
          m_playerColor(playerColor) {}

    void run() override
    {
        QMutexLocker locker(&m_mutex);
        std::vector<Token> tokens;
        tokens.reserve(m_tokensPerPlayer);

        // Debug output
        qDebug() << "Initializing" << m_tokensPerPlayer << "tokens for player" << m_playerId;
        qDebug() << "Token type:" << m_tokenType;
        qDebug() << "Player color:" << m_playerColor.name();

        // Get home position based on color
        QPair<int, int> homePos = getHomePosition(m_playerColor);
        int startX = homePos.first;
        int startY = homePos.second;

        // Initialize tokens
        for (int i = 0; i < m_tokensPerPlayer; ++i)
        {
            Token token;
            token.x = startX + (i % 2);
            token.y = startY + (i / 2);
            token.color = m_playerColor;
            token.isHome = true;
            token.inSafeZone = false;
            token.tokenType = m_tokenType; // Make sure this is set

            qDebug() << "Created token at" << token.x << "," << token.y
                     << "of type" << token.tokenType;

            tokens.push_back(token);
        }

        m_playerTokens[m_playerId] = tokens;
    }

private:
    QPair<int, int> getHomePosition(const QColor &color)
    {
        // For 2P mode
        if (m_playerId == 0)
        {                           // First player
            return qMakePair(1, 1); // Top-left home
        }
        else if (m_playerId == 1)
        {                            // Second player
            return qMakePair(12, 1); // Top-right home
        }
        // For 4P mode
        else if (m_playerId == 2)
        {
            return qMakePair(1, 12); // Bottom-left home
        }
        else
        {
            return qMakePair(12, 12); // Bottom-right home
        }
    }
    std::map<int, std::vector<Token>> &m_playerTokens;
    int m_playerId;
    int m_tokensPerPlayer;
    QString m_tokenType;
    static QMutex m_mutex;
    QColor m_playerColor;
};

class PlayerSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlayerSelectionDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Game Setup");
        setStyleSheet(
            "QDialog { "
            "    background-color: #2A0944;"
            "    border-radius: 10px;"
            "}"
            "QLabel { "
            "    color: white; "
            "    font-size: 18px;"
            "    font-weight: bold;"
            "}");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(20);
        mainLayout->setContentsMargins(30, 30, 30, 30);

        // Title
        QLabel *titleLabel = new QLabel("SELECT ANY MODE", this);
        titleLabel->setStyleSheet(
            "font-size: 24px;"
            "color: #FFD700;"
            "padding: 10px;");
        mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);

        // Players Selection
        QLabel *playersLabel = new QLabel("Select Players:", this);
        mainLayout->addWidget(playersLabel);

        QHBoxLayout *playerButtonsLayout = new QHBoxLayout();
        m_2PlayersRadio = new QRadioButton("2 PLAYERS", this);
        m_4PlayersRadio = new QRadioButton("4 PLAYERS", this);

        styleRadioButton(m_2PlayersRadio);
        styleRadioButton(m_4PlayersRadio);

        playerButtonsLayout->addWidget(m_2PlayersRadio);
        playerButtonsLayout->addWidget(m_4PlayersRadio);
        mainLayout->addLayout(playerButtonsLayout);

        // Play Button
        QPushButton *playButton = createPlayButton();
        mainLayout->addWidget(playButton);

        // Connect signals
        connect(m_2PlayersRadio, &QRadioButton::toggled, this, [this](bool checked)
                {
                if (checked) m_selectedPlayerCount = 2; });
        connect(m_4PlayersRadio, &QRadioButton::toggled, this, [this](bool checked)
                {
                if (checked) m_selectedPlayerCount = 4; });
        connect(playButton, &QPushButton::clicked, this, &QDialog::accept);

        // Set default selection
        m_2PlayersRadio->setChecked(true);
        m_selectedPlayerCount = 2;
    }
    int getPlayerCount() const { return m_selectedPlayerCount; }
    QColor getSelectedColor() const
    {
        if (m_selectedPlayerCount != 2)
            return Qt::red; // Default for 4-player mode

        QString color = m_colorComboBox->currentText();
        if (color == "Red")
            return Qt::red;
        if (color == "Blue")
            return Qt::blue;
        if (color == "Green")
            return Qt::green;
        if (color == "Yellow")
            return Qt::yellow;
        return Qt::red;
    }

signals:
    void gameConfigured(int playerCount, QString token, QColor color, int tokenCount);

private slots:
    void onPlayerModeChanged(bool checked)
    {
        if (!checked)
            return;
        m_selectedPlayerCount = 2;
        bool is2PlayerMode = true;
        m_colorLabel->setVisible(is2PlayerMode);
        m_colorComboBox->setVisible(is2PlayerMode);
    }

    void onPlayClicked()
    {
        emit gameConfigured(m_selectedPlayerCount,
                            m_tokenComboBox->currentText(),
                            getSelectedColor(),
                            m_tokenSpinBox->value());
        accept();
    }

private:
    void styleRadioButton(QRadioButton *radio)
    {
        radio->setStyleSheet(
            "QRadioButton {"
            "    color: white;"
            "    font-size: 16px;"
            "    font-weight: bold;"
            "    padding: 10px;"
            "    background-color: #4A235A;"
            "    border: 2px solid #6C3483;"
            "    border-radius: 8px;"
            "}"
            "QRadioButton:checked {"
            "    background-color: #8E44AD;"
            "    border-color: #9B59B6;"
            "}"
            "QRadioButton:hover {"
            "    background-color: #6C3483;"
            "}"
            "QRadioButton::indicator {"
            "    width: 0px;"
            "    height: 0px;"
            "}");
        radio->setMinimumSize(150, 50);
    }
    void setupTokenQuantitySelection()
    {
        QLabel *quantityLabel = new QLabel("Number of Tokens per Player:", this);
        quantityLabel->setStyleSheet("color: white; font-size: 16px;");

        m_tokenQuantitySpinBox = new QSpinBox(this);
        m_tokenQuantitySpinBox->setRange(1, 4);
        m_tokenQuantitySpinBox->setValue(4);
        m_tokenQuantitySpinBox->setStyleSheet(
            "QSpinBox {"
            "    background-color: #4A235A;"
            "    color: white;"
            "    border: 2px solid #6C3483;"
            "    border-radius: 8px;"
            "    padding: 8px;"
            "    font-size: 16px;"
            "}");

        mainLayout->addWidget(quantityLabel);
        mainLayout->addWidget(m_tokenQuantitySpinBox);
    }
    QPushButton *createPlayButton()
    {
        QPushButton *button = new QPushButton("PLAY");
        button->setStyleSheet(
            "QPushButton {"
            "    background-color: #27AE60;"
            "    color: white;"
            "    border: none;"
            "    border-radius: 8px;"
            "    font-size: 18px;"
            "    font-weight: bold;"
            "    padding: 15px;"
            "    margin-top: 20px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #2ECC71;"
            "}"
            "QPushButton:pressed {"
            "    background-color: #219A52;"
            "}");
        return button;
    }

    QString getComboBoxStyle()
    {
        return "QComboBox {"
               "    background-color: #4A235A;"
               "    color: white;"
               "    border: 2px solid #6C3483;"
               "    border-radius: 8px;"
               "    padding: 8px;"
               "    font-size: 16px;"
               "}"
               "QComboBox::drop-down {"
               "    border: none;"
               "    width: 30px;"
               "}"
               "QComboBox::down-arrow {"
               "    image: url(:/images/down-arrow.png);"
               "    width: 12px;"
               "    height: 12px;"
               "}"
               "QComboBox QAbstractItemView {"
               "    background-color: #4A235A;"
               "    color: white;"
               "    selection-background-color: #6C3483;"
               "    border: 1px solid #6C3483;"
               "}";
    }

    QRadioButton *m_2PlayersRadio;
    QRadioButton *m_4PlayersRadio;
    QComboBox *m_tokenComboBox;
    QComboBox *m_colorComboBox;
    QLabel *m_colorLabel;
    int m_selectedPlayerCount;
    QSpinBox *m_tokenSpinBox;
    QSpinBox *m_tokenQuantitySpinBox;
    QVBoxLayout *mainLayout;
};

QMutex PlayerInitializer::m_mutex;

class PlayerIndicator : public QWidget
{
    Q_OBJECT
public:
    PlayerIndicator(const QString &name, const QColor &color, QWidget *parent = nullptr) : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setSpacing(8);
        layout->setContentsMargins(8, 4, 8, 4);

        // Active indicator
        m_activeIndicator = new QLabel("▶");
        m_activeIndicator->setStyleSheet(
            "color: gold;"
            "font-size: 18px;"
            "font-weight: bold;");
        m_activeIndicator->setVisible(false);

        // Color indicator
        QFrame *colorBox = new QFrame;
        colorBox->setFixedSize(16, 16);
        colorBox->setStyleSheet(QString(
                                    "background-color: %1;"
                                    "border-radius: 8px;"
                                    "border: 2px solid white;")
                                    .arg(color.name()));

        // Player name
        QLabel *nameLabel = new QLabel(name);
        nameLabel->setStyleSheet(
            "color: white;"
            "font-size: 14px;");

        layout->addWidget(m_activeIndicator);
        layout->addWidget(colorBox);
        layout->addWidget(nameLabel, 1);
        layout->addStretch();

        setStyleSheet(
            "background-color: rgba(255, 255, 255, 0.1);"
            "border-radius: 8px;"
            "margin: 2px;");
    }

    void setActive(bool active)
    {
        m_activeIndicator->setVisible(active);
        setStyleSheet(QString(
                          "background-color: rgba(255, 255, 255, %1);"
                          "border-radius: 8px;"
                          "margin: 2px;")
                          .arg(active ? "0.2" : "0.1"));
    }

private:
    QFrame *m_colorBox;
    QLabel *m_nameLabel;
    QLabel *m_activeIndicator;
};

class PlayerScoreWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerScoreWidget(int rank, const QColor &color, int score, int hitRate, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);

        // Rank medal
        QLabel *medalLabel = new QLabel;
        QString medalStyle;
        switch (rank)
        {
        case 1:
            medalStyle = "🏆";
            break;
        case 2:
            medalStyle = "🥈";
            break;
        case 3:
            medalStyle = "🥉";
            break;
        default:
            medalStyle = QString::number(rank);
        }
        medalLabel->setText(medalStyle);
        medalLabel->setStyleSheet("font-size: 24px;");

        // Color indicator
        QFrame *colorIndicator = new QFrame;
        colorIndicator->setFixedSize(30, 30);
        colorIndicator->setStyleSheet(QString("background-color: %1; border-radius: 15px; border: 2px solid white;")
                                          .arg(color.name()));

        // Score and hit rate
        QLabel *scoreLabel = new QLabel(QString("Score: %1").arg(score));
        QLabel *hitRateLabel = new QLabel(QString("Hits: %1").arg(hitRate));

        QString textStyle = "color: white; font-size: 16px; font-weight: bold;";
        scoreLabel->setStyleSheet(textStyle);
        hitRateLabel->setStyleSheet(textStyle);

        layout->addWidget(medalLabel);
        layout->addWidget(colorIndicator);
        layout->addWidget(scoreLabel);
        layout->addWidget(hitRateLabel);
        layout->addStretch();

        // Add glow effect
        QGraphicsDropShadowEffect *glowEffect = new QGraphicsDropShadowEffect;
        glowEffect->setBlurRadius(20);
        glowEffect->setColor(color);
        glowEffect->setOffset(0);
        this->setGraphicsEffect(glowEffect);

        setStyleSheet("background-color: rgba(0, 0, 0, 40%); border-radius: 15px; padding: 10px;");
    }
};
class WinningPosition : public QWidget
{
    Q_OBJECT
public:
    WinningPosition(int rank, int playerId, QWidget *parent = nullptr) : QWidget(parent)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(8, 4, 8, 4);
        layout->setSpacing(8);

        // Rank with medal
        QString medalEmoji;
        QString rankText;
        switch (rank)
        {
        case 1:
            medalEmoji = "🥇";
            rankText = "1st";
            break;
        case 2:
            medalEmoji = "🥈";
            rankText = "2nd";
            break;
        case 3:
            medalEmoji = "🥉";
            rankText = "3rd";
            break;
        default:
            medalEmoji = "🏅";
            rankText = QString("%1th").arg(rank);
        }

        QLabel *rankLabel = new QLabel(QString("%1 %2").arg(medalEmoji).arg(rankText));
        rankLabel->setStyleSheet(
            "color: gold;"
            "font-size: 14px;"
            "font-weight: bold;");

        // Player color indicator
        QFrame *colorBox = new QFrame;
        colorBox->setFixedSize(16, 16);
        QColor playerColor = getPlayerColor(playerId);
        colorBox->setStyleSheet(QString(
                                    "background-color: %1;"
                                    "border-radius: 8px;"
                                    "border: 2px solid white;")
                                    .arg(playerColor.name()));

        layout->addWidget(rankLabel);
        layout->addWidget(colorBox);
        layout->addStretch();

        setStyleSheet(
            "background-color: rgba(255, 255, 255, 0.1);"
            "border-radius: 8px;"
            "margin: 2px;");

        setFixedHeight(32);
    }

private:
    QColor getPlayerColor(int playerId) const
    {
        switch (playerId)
        {
        case 0:
            return Qt::blue;
        case 1:
            return Qt::red;
        case 2:
            return Qt::yellow;
        case 3:
            return Qt::green;
        default:
            return Qt::white;
        }
    }
};

class WinningBox : public QWidget
{
    Q_OBJECT
public:
    WinningBox(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(4);

        // Title
        QLabel *titleLabel = new QLabel("Winnings");
        titleLabel->setStyleSheet(
            "color: gold;"
            "font-size: 16px;"
            "font-weight: bold;"
            "padding: 4px;");
        titleLabel->setAlignment(Qt::AlignCenter);

        // Scroll area for positions
        QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setStyleSheet(
            "QScrollArea { background: transparent; border: none; }"
            "QScrollBar:vertical { width: 8px; background: #001a33; }"
            "QScrollBar::handle:vertical { background: #0059b3; border-radius: 4px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

        m_container = new QWidget;
        m_layout = new QVBoxLayout(m_container);
        m_layout->setContentsMargins(2, 2, 2, 2);
        m_layout->setSpacing(4);
        m_layout->addStretch();

        scrollArea->setWidget(m_container);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(scrollArea);

        setStyleSheet(
            "background-color: #002244;"
            "border-radius: 8px;");
        setFixedWidth(120);
    }

    void updatePositions(const QVector<QPair<int, int>> &positions)
    {
        // Clear existing positions
        QLayoutItem *item;
        while ((item = m_layout->takeAt(0)) != nullptr)
        {
            delete item->widget();
            delete item;
        }

        // Sort positions by rank
        QVector<QPair<int, int>> sortedPositions = positions;
        std::sort(sortedPositions.begin(), sortedPositions.end(),
                  [](const QPair<int, int> &a, const QPair<int, int> &b)
                  {
                      return a.second < b.second;
                  });

        // Add positions in sorted order
        for (const auto &pos : sortedPositions)
        {
            WinningPosition *posWidget = new WinningPosition(pos.second, pos.first);
            m_layout->insertWidget(m_layout->count(), posWidget);
        }

        // Add stretch at the end
        m_layout->addStretch();
    }

private:
    QVBoxLayout *m_layout;
    QWidget *m_container;
};
class HamburgerMenu : public QWidget
{
    Q_OBJECT

public:
    explicit HamburgerMenu(QWidget *parent = nullptr) : QWidget(parent)
    {
        QPushButton *menuButton = new QPushButton(this);
        menuButton->setFixedSize(40, 40);
        menuButton->setStyleSheet(
            "QPushButton {"
            "    background-color: transparent;"
            "    border: none;"
            "    padding: 5px;"
            "}"
            "QPushButton:hover {"
            "    background-color: rgba(255, 255, 255, 0.15);"
            "    border-radius: 20px;"
            "}"
            "QPushButton:pressed {"
            "    background-color: rgba(255, 255, 255, 0.1);"
            "}");

        // Create custom menu icon
        QPixmap pixmap(40, 40);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw dice-inspired menu icon
        QPen pen(QColor("#FFD700")); // Golden color
        pen.setWidth(2);
        painter.setPen(pen);

        // Create gradient for the background
        QLinearGradient gradient(0, 0, 40, 40);
        gradient.setColorAt(0, QColor(42, 9, 68));    // #2A0944
        gradient.setColorAt(1, QColor(161, 37, 104)); // #A12568

        // Draw rounded rectangle background
        painter.setBrush(gradient);
        painter.drawRoundedRect(5, 5, 30, 30, 8, 8);

        // Draw dice dots in a menu pattern
        painter.setBrush(QColor("#FFD700"));

        // First row of dots
        painter.drawEllipse(12, 12, 4, 4);
        painter.drawEllipse(19, 12, 4, 4);
        painter.drawEllipse(26, 12, 4, 4);

        // Second row of dots
        painter.drawEllipse(12, 19, 4, 4);
        painter.drawEllipse(19, 19, 4, 4);
        painter.drawEllipse(26, 19, 4, 4);

        // Third row of dots
        painter.drawEllipse(12, 26, 4, 4);
        painter.drawEllipse(19, 26, 4, 4);
        painter.drawEllipse(26, 26, 4, 4);

        // Add subtle glow effect
        QRadialGradient glowGradient(20, 20, 25);
        glowGradient.setColorAt(0, QColor(255, 215, 0, 30));
        glowGradient.setColorAt(1, QColor(255, 215, 0, 0));
        painter.setBrush(glowGradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(5, 5, 30, 30);

        menuButton->setIcon(QIcon(pixmap));
        menuButton->setIconSize(QSize(40, 40));

        QMenu *menu = new QMenu(this);
        menu->setStyleSheet(
            "QMenu {"
            "    background-color: #2A0944;"
            "    border: 2px solid #FFD700;"
            "    border-radius: 10px;"
            "    padding: 5px;"
            "}"
            "QMenu::item {"
            "    color: #FFD700;"
            "    padding: 8px 20px;"
            "    border-radius: 5px;"
            "    margin: 2px 5px;"
            "}"
            "QMenu::item:selected {"
            "    background-color: #A12568;"
            "    color: white;"
            "}"
            "QMenu::separator {"
            "    height: 1px;"
            "    background: #FFD700;"
            "    margin: 5px 15px;"
            "}");

        QAction *mainMenuAction = new QAction("🏠 Main Menu", menu);
        QAction *restartAction = new QAction("🔄 Restart Game", menu);
        QAction *quitAction = new QAction("🚪 Quit Game", menu);

        menu->addAction(mainMenuAction);
        menu->addAction(restartAction);
        menu->addSeparator();
        menu->addAction(quitAction);

        connect(menuButton, &QPushButton::clicked, [=]()
                {
            QPoint pos = menuButton->mapToGlobal(QPoint(0, menuButton->height()));
            menu->popup(pos); });

        connect(mainMenuAction, &QAction::triggered, this, &HamburgerMenu::mainMenuRequested);
        connect(restartAction, &QAction::triggered, this, &HamburgerMenu::restartRequested);
        connect(quitAction, &QAction::triggered, this, &HamburgerMenu::quitRequested);

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(menuButton);
    }

signals:
    void mainMenuRequested();
    void restartRequested();
    void quitRequested();
};

class SidePanel : public QWidget
{
    Q_OBJECT
public:
    explicit SidePanel(QWidget *parent = nullptr) : QWidget(parent)
    {
        setFixedWidth(300);
        setStyleSheet("background-color: #001a33;");

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(10, 20, 10, 20);

        // Add hamburger menu to top-right
        QHBoxLayout *topBar = new QHBoxLayout;
        topBar->addStretch();

        m_hamburgerMenu = new HamburgerMenu(this);
        topBar->addWidget(m_hamburgerMenu);
        mainLayout->addLayout(topBar);

        // Player indicators
        QWidget *playerContainer = new QWidget;
        QVBoxLayout *playerLayout = new QVBoxLayout(playerContainer);
        playerLayout->setSpacing(4);
        playerLayout->setContentsMargins(4, 4, 4, 4);

        const QVector<QPair<QString, QColor>> players = {
            {"Jinx", Qt::blue},
            {"Ekko", Qt::red},
            {"Vi", Qt::yellow},
            {"Caitlyn", Qt::green}};

        for (const auto &player : players)
        {
            PlayerIndicator *indicator = new PlayerIndicator(player.first, player.second);
            m_playerIndicators.append(indicator);
            playerLayout->addWidget(indicator);
        }

        playerContainer->setStyleSheet(
            "background-color: #002244;"
            "border-radius: 8px;");

        mainLayout->addWidget(playerContainer);

        // Create winning box
        m_winningBox = new WinningBox;
        mainLayout->addWidget(m_winningBox, 0, Qt::AlignRight);
        mainLayout->addStretch();

        // Throw dice button
        m_throwDiceBtn = new QPushButton("Throw Dice");
        m_throwDiceBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #0059b3;"
            "    color: white;"
            "    border: none;"
            "    padding: 15px;"
            "    border-radius: 5px;"
            "    font-size: 16px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #0066cc; }"
            "QPushButton:pressed { background-color: #004d99; }");
        mainLayout->addWidget(m_throwDiceBtn);

        // Connect hamburger menu signals
        connect(m_hamburgerMenu, &HamburgerMenu::mainMenuRequested, this, &SidePanel::mainMenuRequested);
        connect(m_hamburgerMenu, &HamburgerMenu::restartRequested, this, &SidePanel::restartRequested);
        connect(m_hamburgerMenu, &HamburgerMenu::quitRequested, this, &SidePanel::quitRequested);
    }

    void updateWinningPositions(const QVector<QPair<int, int>> &positions)
    {
        m_winningBox->updatePositions(positions);
    }

    void setActivePlayer(int index)
    {
        for (int i = 0; i < m_playerIndicators.size(); ++i)
        {
            m_playerIndicators[i]->setActive(i == index);
        }
    }

    QPushButton *throwDiceButton() const { return m_throwDiceBtn; }
signals:
    void mainMenuRequested();
    void restartRequested();
    void quitRequested();

private:
    WinningBox *m_winningBox;
    QPushButton *m_throwDiceBtn;
    QVector<PlayerIndicator *> m_playerIndicators;
    HamburgerMenu *m_hamburgerMenu;
};

class LudoBoard : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal diceRotation READ diceRotation WRITE setDiceRotation)
    Q_PROPERTY(QPointF dicePos READ dicePos WRITE setDicePos)
    Q_PROPERTY(qreal diceScale READ diceScale WRITE setDiceScale)
public:
    explicit LudoBoard(bool is2PMode, int tokenCount, QColor p1Color, QColor p2Color, QString tokenType, QWidget *parent = nullptr)
        : QWidget(parent), isRolling(false),
          currentPlayer(0),
          waitingForMove(false),
          m_isRolling(false),
          m_diceVisible(false),
          m_diceRotation(0),
          m_diceScale(1.0),
          consecutiveSixes(0),
          bonusRolls(0),
          diceValue(1),
          pendingMoves(0),
          turnTimeLeft(30),
          turnExpired(false),
          m_is2PMode(is2PMode),
          m_tokenCount(tokenCount),    // Store token count
          tokensPerPlayer(tokenCount), // Set tokensPerPlayer to actual count
          m_p1Color(p1Color),
          m_p2Color(p2Color),
          m_tokenType(tokenType)
    {
        // Initialize remaining players for the first iteration
        initializeNewIteration();

        // Set up the main layout
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Create and set up the board container
        QWidget *boardContainer = new QWidget;
        boardContainer->setFixedSize(GRID_SIZE * TILE_SIZE, GRID_SIZE * TILE_SIZE);
        mainLayout->addWidget(boardContainer);

        // Initialize the side panel
        m_sidePanel = new SidePanel(this);
        mainLayout->addWidget(m_sidePanel);

        // Connect the throw dice button
        connect(m_sidePanel->throwDiceButton(), &QPushButton::clicked, this, &LudoBoard::startDiceRoll);

        // Initialize timers
        rollTimer = new QTimer(this);
        connect(rollTimer, &QTimer::timeout, this, &LudoBoard::updateDiceRotation);
        rollTimer->setInterval(20);

        turnTimer = new QTimer(this);
        connect(turnTimer, &QTimer::timeout, this, &LudoBoard::onTurnExpired);
        turnTimer->setInterval(1000);

        // Initialize game paths
        initializePaths();

        // Initialize game mode
        if (m_is2PMode)
        {
            initialize2PlayerMode();
        }
        else
        {
            initialize4PlayerMode();
        }

        // Initialize animation state
        animState.timer = new QTimer(this);
        connect(animState.timer, &QTimer::timeout, this, &LudoBoard::updateAnimation);

        // Clear and initialize player rankings
        playerRankings.clear();
        for (int i = 0; i < (m_is2PMode ? 2 : MAX_PLAYERS); i++)
        {
            finishedPlayers[i] = false;
        }

        // Set focus policy and start timers
        setFocusPolicy(Qt::StrongFocus);
        turnTimer->start();

        // Initialize player statistics
        initializePlayerStats();

        // Initialize consecutive turns counter
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            consecutiveTurnsWithoutAction[i] = 0;
        }

        // Initialize player hit counts
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            playerHitCounts[i] = 0;
        }

        // Initialize finished tokens count
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            finishedTokensCount[i] = 0;
        }

        // Initialize animation state
        animState.isAnimating = false;
        animState.token = nullptr;
        animState.capturedToken = nullptr;
        animState.capturedPlayer = -1;
        animState.currentStep = 0;
        animState.targetX = 0;
        animState.targetY = 0;

        // Set initial dice state
        m_dicePos = QPointF(width() - 150, height() - 60);
    }
    ~LudoBoard()
    {
        if (rollTimer)
        {
            delete rollTimer;
            rollTimer = nullptr;
        }
        if (turnTimer)
        {
            delete turnTimer;
            turnTimer = nullptr;
        }
        if (animState.timer)
        {
            delete animState.timer;
            animState.timer = nullptr;
        }
    }
    SidePanel *getSidePanel() const { return m_sidePanel; }
    void updateTokenPosition(int player, int tokenIndex, int newX, int newY)
    {
        if (playerTokens.find(player) != playerTokens.end() && tokenIndex < playerTokens[player].size())
        {
            playerTokens[player][tokenIndex].x = newX;
            playerTokens[player][tokenIndex].y = newY;
            update(); // Trigger repaint
        }
    }
signals:
    void diceRotationChanged();
    void dicePosChanged();
    void diceScaleChanged();

protected:
    bool isTokenInSafeZone(int x, int y)
    {
        // Define safe zones coordinates
        return (x == 1 && y == 6) ||  // Blue safe zone
               (x == 8 && y == 1) ||  // Red safe zone
               (x == 6 && y == 13) || // Yellow safe zone
               (x == 13 && y == 8);   // Green safe zone
    }
    void drawDice(QPainter &painter)
    {
        const int DICE_SIZE = 40;
        painter.save();
        painter.translate(GRID_SIZE * TILE_SIZE + 50, (GRID_SIZE * TILE_SIZE) / 2 - 40);

        if (isRolling)
        {
            painter.rotate(gameDiceRotation);
        }

        painter.fillRect(QRectF(-DICE_SIZE / 2, -DICE_SIZE / 2, DICE_SIZE, DICE_SIZE), QColor(255, 255, 255));
        painter.setPen(QPen(Qt::black, 2));
        painter.drawRect(QRectF(-DICE_SIZE / 2, -DICE_SIZE / 2, DICE_SIZE, DICE_SIZE));

        QVector<QPointF> dotPositions;
        switch (diceValue)
        {
        case 1:
            dotPositions = {{0, 0}};
            break;
        case 2:
            dotPositions = {{-10, -10}, {10, 10}};
            break;
        case 3:
            dotPositions = {{-10, -10}, {0, 0}, {10, 10}};
            break;
        case 4:
            dotPositions = {{-10, -10}, {10, -10}, {-10, 10}, {10, 10}};
            break;
        case 5:
            dotPositions = {{-10, -10}, {10, -10}, {0, 0}, {-10, 10}, {10, 10}};
            break;
        case 6:
            dotPositions = {{-10, -10}, {-10, 0}, {-10, 10}, {10, -10}, {10, 0}, {10, 10}};
            break;
        }

        painter.setBrush(Qt::black);
        for (const auto &pos : dotPositions)
        {
            painter.drawEllipse(pos, 4, 4);
        }

        painter.restore();
    }

    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        drawLudoBoard(painter);
        drawPlayerRankings(painter);
        drawPlayerStats(painter); // Draw player stats

        // Replace this section
        drawTokens(painter); // Use the TokenDrawer class to draw tokens

        if (m_isRolling || m_diceVisible)
        {
            drawAnimatedDice(painter);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        QRectF diceArea(GRID_SIZE * TILE_SIZE + 20, (GRID_SIZE * TILE_SIZE) / 2 - 50, 100, 100);
        if (diceArea.contains(event->pos()))
        {
            if (!isRolling && !waitingForMove)
            {
                startDiceRoll();
            }
            return;
        }

        if (waitingForMove)
        {
            bool moved = false;
            for (Token &token : playerTokens[currentPlayer])
            {
                QRectF tokenArea(token.x * TILE_SIZE + 10, token.y * TILE_SIZE + 10,
                                 TILE_SIZE - 20, TILE_SIZE - 20);

                if (tokenArea.contains(event->pos()))
                {
                    // Check if token is in home
                    if (isTokenInHome(token))
                    {
                        // Only allow movement out of home if dice shows 6
                        if (diceValue == 6)
                        {
                            moveTokenOutOfHome(token);
                            moved = true;
                        }
                        else
                        {
                            // Show message that 6 is needed to move out of home
                            QMessageBox::information(this, "Invalid Move",
                                                     "You need to roll a 6 to move a token out of home!");
                        }
                    }
                    else
                    {
                        // Token is already out, move normally
                        moveTokenAlongPath(token, diceValue);
                        moved = true;
                    }
                    break;
                }
            }

            if (moved)
            {
                waitingForMove = false;
                if (diceValue != 6)
                {
                    consecutiveSixes = 0;
                    advanceTurn();
                }
                else
                {
                    // Allow another roll for the same player
                    isRolling = false;
                }
                update();
            }
        }
    }

private:
    Dice gameDice;
    int currentPlayer = 0; // Set current player for dice color
public:
    std::map<int, std::vector<Token>> playerTokens; // Player tokens mapped by player ID
private:
    void initializeDice()
    {
        gameDice.isRolling = false;
        gameDice.value = 1; // Initial dice value
        gameDice.rotation = 0;
    }
    bool isWinningPosition(const Token &token, int player)
    {
        switch (player)
        {
        case 0: // Blue
            return token.x == 6 && token.y == 7;
        case 1: // Red
            return token.x == 7 && token.y == 6;
        case 2: // Yellow
            return token.x == 7 && token.y == 8;
        case 3: // Green
            return token.x == 8 && token.y == 7;
        default:
            return false;
        }
    }
    bool playerExists(int playerId) const
    {
        return playerTokens.count(playerId) > 0;
    }

    bool canMoveToken(const Token &token, int steps)
    {
        if (isTokenAtWinningPosition(token, currentPlayer))
        {
            return false; // Token is already at winning position, can't move
        }

        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        int pathLength = currentPath->size();
        int stepsToWinning = pathLength - currentIndex - 1;

        if (currentIndex >= pathLength - 6)
        {                                   // Token is on the colored path
            return steps <= stepsToWinning; // Can only move if steps are less than or equal to remaining steps
        }

        return true; // Can move on the main path
    }
    bool isTokenAtWinningPosition(const Token &token, int player)
    {
        return isWinningPosition(token, player);
    }
    bool isWhitePath(int x, int y)
    {
        // Check if the position is a safe zone
        if ((x == 1 && y == 6) ||  // Blue safe zone
            (x == 8 && y == 1) ||  // Red safe zone
            (x == 6 && y == 13) || // Yellow safe zone
            (x == 13 && y == 8))   // Green safe zone
        {
            return false;
        }

        // Check if the position is on the white path
        return ((x == 6 || x == 8) && (y > 0 && y < 14)) ||
               ((y == 6 || y == 8) && (x > 0 && x < 14));
    }
    void implementWinningStrategy(Token &token, int steps)
    {
        if (!canMoveToken(token, steps))
        {
            return; // Don't implement strategy if move is not allowed
        }
        // Priority 1: Move to winning position if possible
        if (canMoveToWinningPosition(token, steps))
        {
            moveToWinningPosition(token);
            return;
        }

        // Priority 2: Move out of home if possible
        if (isTokenInHome(token) && steps == 6)
        {
            moveTokenOutOfHome(token);
            return;
        }

        // Priority 3: Capture opponent's token if possible (only on white path)
        if (canCaptureOpponent(token, steps))
        {
            moveAndCapture(token, steps);
            return;
        }

        // Priority 4: Move towards winning position
        moveTowardsWinningPosition(token, steps);
    }
    bool canMoveToWinningPosition(const Token &token, int steps)
    {
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        int targetIndex = currentPath->size() - 1; // Index of winning position

        return (currentIndex != -1) && (currentIndex + steps == targetIndex);
    }

    void moveToWinningPosition(Token &token)
    {
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int targetIndex = currentPath->size() - 1; // Index of winning position
        token.x = (*currentPath)[targetIndex].x;
        token.y = (*currentPath)[targetIndex].y;
    }
    bool isTokenInHome(const Token &token)
    {
        switch (currentPlayer)
        {
        case 0: // Blue
            return token.x >= 1 && token.x <= 2 && token.y >= 1 && token.y <= 2;
        case 1: // Red
            return token.x >= 12 && token.x <= 13 && token.y >= 1 && token.y <= 2;
        case 2: // Yellow
            return token.x >= 1 && token.x <= 2 && token.y >= 12 && token.y <= 13;
        case 3: // Green
            return token.x >= 12 && token.x <= 13 && token.y >= 12 && token.y <= 13;
        default:
            return false;
        }
    }
    void moveTokenOutOfHome(Token &token)
    {
        // Only move token out if a 6 was rolled
        if (diceValue != 6)
        {
            return;
        }

        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        if (currentPath && !currentPath->empty())
        {
            token.x = (*currentPath)[0].x;
            token.y = (*currentPath)[0].y;
            token.isHome = false;
        }
    }
    bool canCaptureOpponent(const Token &token, int steps)
    {
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        int targetIndex = (currentIndex + steps) % currentPath->size();
        int targetX = (*currentPath)[targetIndex].x;
        int targetY = (*currentPath)[targetIndex].y;

        // Only check for capture on white paths
        if (!isWhitePath(targetX, targetY))
        {
            return false;
        }

        for (int player = 0; player < MAX_PLAYERS; ++player)
        {
            if (player == currentPlayer)
                continue;
            for (const auto &opponentToken : playerTokens[player])
            {
                if (opponentToken.x == targetX && opponentToken.y == targetY)
                {
                    return true;
                }
            }
        }
        return false;
    }
    void moveAndCapture(Token &token, int steps)
    {
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        int targetIndex = (currentIndex + steps) % currentPath->size();

        int newX = (*currentPath)[targetIndex].x;
        int newY = (*currentPath)[targetIndex].y;

        // Only capture on white paths
        if (isWhitePath(newX, newY))
        {
            for (int player = 0; player < MAX_PLAYERS; ++player)
            {
                if (player == currentPlayer)
                    continue;
                for (auto &opponentToken : playerTokens[player])
                {
                    if (opponentToken.x == newX && opponentToken.y == newY)
                    {
                        moveTokenToHome(opponentToken, player);
                        break;
                    }
                }
            }
        }

        // Move the current token
        token.x = newX;
        token.y = newY;
        playerStats[currentPlayer].hits++;
    }

    void moveTowardsWinningPosition(Token &token, int steps)
    {
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        int targetIndex = (currentIndex + steps) % currentPath->size();

        token.x = (*currentPath)[targetIndex].x;
        token.y = (*currentPath)[targetIndex].y;
    }

    std::vector<PathCoordinate> *getPathForPlayer(int player)
    {
        switch (player)
        {
        case 0:
            return &bluePath;
        case 1:
            return &redPath;
        case 2:
            return &yellowPath;
        case 3:
            return &greenPath;
        default:
            return nullptr;
        }
    }

    int findTokenIndexInPath(const Token &token, const std::vector<PathCoordinate> &path)
    {
        for (int i = 0; i < path.size(); ++i)
        {
            if (path[i].x == token.x && path[i].y == token.y)
            {
                return i;
            }
        }
        return -1; // Token not found in path
    }

    void moveTokenToHome(Token &token, int player)
    {
        token.isHome = true;

        if (m_is2PMode)
        {
            // In 2P mode, only use top-left and top-right homes
            if (player == 0)
            { // First player (top-left)
                token.x = 1 + (QRandomGenerator::global()->bounded(2));
                token.y = 1 + (QRandomGenerator::global()->bounded(2));
            }
            else
            { // Second player (top-right)
                token.x = 12 + (QRandomGenerator::global()->bounded(2));
                token.y = 1 + (QRandomGenerator::global()->bounded(2));
            }
        }
        else
        {
            // Original 4P home positions
            switch (player)
            {
            case 0: // Blue
                token.x = 1 + (QRandomGenerator::global()->bounded(2));
                token.y = 1 + (QRandomGenerator::global()->bounded(2));
                break;
            case 1: // Red
                token.x = 12 + (QRandomGenerator::global()->bounded(2));
                token.y = 1 + (QRandomGenerator::global()->bounded(2));
                break;
            case 2: // Yellow
                token.x = 1 + (QRandomGenerator::global()->bounded(2));
                token.y = 12 + (QRandomGenerator::global()->bounded(2));
                break;
            case 3: // Green
                token.x = 12 + (QRandomGenerator::global()->bounded(2));
                token.y = 12 + (QRandomGenerator::global()->bounded(2));
                break;
            }
        }
        token.inSafeZone = false;
    }

    void onTurnExpired()
    {
        if (turnTimeLeft > 0)
        {
            turnTimeLeft--;
            update();

            if (turnTimeLeft == 0)
            {
                // If time runs out, force end turn
                moveValues.clear();
                pendingMoves = 0;
                waitingForMove = false;
                advanceTurn();
            }
        }
    }

    void updateDiceRotation()
    {
        if (isRolling)
        {
            gameDiceRotation += 15.0f;
            if (gameDiceRotation >= 360.0f)
            {
                gameDiceRotation = 0.0f;
                stopDiceRoll();
            }
            update();
        }
    }
    void startDiceRoll()
    {
        // First check if current player has completed the game
        bool playerCompleted = true;
        for (const Token &token : playerTokens[currentPlayer])
        {
            if (!isWinningPosition(token, currentPlayer))
            {
                playerCompleted = false;
                break;
            }
        }

        // If player has completed, skip their turn
        if (playerCompleted)
        {
            advanceTurn();
            return;
        }

        if (m_isRolling || waitingForMove)
            return;

        m_isRolling = true;
        m_diceVisible = true;
        m_diceRotation = 0;
        m_diceScale = 1.0;

        // Get the exact position of the Throw Dice button
        QPointF buttonCenter(width() - 150, height() - 60);

        // Create sequential animation group for controlled movement
        QSequentialAnimationGroup *sequence = new QSequentialAnimationGroup(this);

        // First phase: Emerge from button
        QPropertyAnimation *emergeAnim = new QPropertyAnimation(this, "dicePos");
        emergeAnim->setDuration(300);
        emergeAnim->setStartValue(buttonCenter);
        emergeAnim->setEndValue(QPointF(buttonCenter.x(), buttonCenter.y() - 50));
        emergeAnim->setEasingCurve(QEasingCurve::OutQuad);

        // Second phase: Roll upward with rotation
        QParallelAnimationGroup *rollGroup = new QParallelAnimationGroup;

        // Vertical movement
        QPropertyAnimation *rollAnim = new QPropertyAnimation(this, "dicePos");
        rollAnim->setDuration(700);
        rollAnim->setStartValue(QPointF(buttonCenter.x(), buttonCenter.y() - 50));
        rollAnim->setEndValue(QPointF(buttonCenter.x(), buttonCenter.y() - 200));
        rollAnim->setEasingCurve(QEasingCurve::OutBounce);

        // Rotation during roll
        QPropertyAnimation *rotateAnim = new QPropertyAnimation(this, "diceRotation");
        rotateAnim->setDuration(1000);
        rotateAnim->setStartValue(0);
        rotateAnim->setEndValue(720 + QRandomGenerator::global()->bounded(360));
        rotateAnim->setEasingCurve(QEasingCurve::OutQuad);
        rotateAnim->start();

        rollGroup->addAnimation(rollAnim);
        rollGroup->addAnimation(rotateAnim);

        sequence->addAnimation(emergeAnim);
        sequence->addAnimation(rollGroup);

        connect(sequence, &QSequentialAnimationGroup::finished, this, &LudoBoard::finishDiceRoll);
        sequence->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void finishDiceRoll()
    {
        m_isRolling = false;
        diceValue = QRandomGenerator::global()->bounded(1, 7);
        handleDiceRoll(diceValue);
        QTimer::singleShot(1000, this, [this]()
                           {
                m_diceVisible = false;
                update(); });
    }

    void handleDiceRoll(int value)
    {
        // If current player has no active tokens, skip their turn
        if (!hasActiveTokens(currentPlayer))
        {
            advanceTurn();
            return;
        }

        diceValue = value;

        if (diceValue == 6)
        {
            if (consecutiveSixes == 0)
            {
                TurnState initialState;
                initialState.tokenPositions = playerTokens[currentPlayer];
                initialState.diceValue = 0;
                turnHistory.push(initialState);
            }

            consecutiveSixes++;
            if (consecutiveSixes == 3)
            {
                revertConsecutiveSixesMoves();
                displayLostTurnMessage();
                consecutiveSixes = 0;
                waitingForMove = false;
                advanceTurn();
                return;
            }
        }
        else
        {
            consecutiveSixes = 0;
            while (!turnHistory.empty())
            {
                turnHistory.pop();
            }
        }

        bool canMove = false;
        bool hitOpponent = false;
        int targetX = -1, targetY = -1; // Initialize target coordinates

        for (const Token &token : playerTokens[currentPlayer])
        {
            if (!isTokenInHome(token) || diceValue == 6)
            {
                canMove = true;
                targetX = token.x; // Set target coordinates
                targetY = token.y;
                break;
            }
        }

        // Check if the player hit an opponent's token
        for (int player = 0; player < MAX_PLAYERS; ++player)
        {
            if (player == currentPlayer)
                continue;
            for (const auto &opponentToken : playerTokens[player])
            {
                if (opponentToken.x == targetX && opponentToken.y == targetY)
                {
                    hitOpponent = true;
                    break;
                }
            }
            if (hitOpponent)
                break;
        }

        if (!canMove && !hitOpponent)
        {
            consecutiveTurnsWithoutAction[currentPlayer]++;
        }
        else
        {
            consecutiveTurnsWithoutAction[currentPlayer] = 0;
        }

        // Check if the player should be kicked out
        if (consecutiveTurnsWithoutAction[currentPlayer] >= 20)
        {
            kickPlayerOut(currentPlayer);
            consecutiveTurnsWithoutAction[currentPlayer] = 0;
            advanceTurn();
            return;
        }

        if (canMove)
        {
            waitingForMove = true;
        }
        else
        {
            consecutiveSixes = 0;
            advanceTurn();
        }

        update();
    }

    void kickPlayerOut(int playerId)
    {
        // Remove the player's tokens from the game
        playerTokens.erase(playerId);

        // Update UI to reflect the change
        update();

        // Display a message to inform the player
        QMessageBox::information(this, "Player Kicked Out",
                                 QString("Player %1 has been kicked out of the game for not rolling a 6 or hitting an opponent's token for 20 consecutive turns.")
                                     .arg(playerId + 1));
        cout << "Threads cancelled for player " << playerId << endl;
    }

    void drawAnimatedDice(QPainter &painter)
    {
        painter.save();

        // Ensure the dice stays in the white area
        QPointF constrainedPos = m_dicePos;
        constrainedPos.setX(width() - 200); // Keep X position fixed
        constrainedPos.setY(qMin(height() - 50.0, qMax(height() - 300.0, constrainedPos.y())));

        // Set up transformations
        painter.translate(constrainedPos);
        painter.rotate(m_diceRotation);
        painter.scale(m_diceScale, m_diceScale);

        // Draw dice
        QRect diceRect(-20, -20, 40, 40); // Slightly smaller dice
        painter.setPen(Qt::black);
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(diceRect, 8, 8);

        // Draw dots based on current value
        painter.setBrush(Qt::black);
        drawDiceDots(painter, diceRect, diceValue);

        painter.restore();
    }

    void drawDiceDots(QPainter &painter, const QRect &rect, int value)
    {
        const int dotSize = 6;
        QVector<QPointF> dots;

        switch (value)
        {
        case 1:
            dots << QPointF(0, 0);
            break;
        case 2:
            dots << QPointF(-10, -10) << QPointF(10, 10);
            break;
        case 3:
            dots << QPointF(-10, -10) << QPointF(0, 0) << QPointF(10, 10);
            break;
        case 4:
            dots << QPointF(-10, -10) << QPointF(10, -10)
                 << QPointF(-10, 10) << QPointF(10, 10);
            break;
        case 5:
            dots << QPointF(-10, -10) << QPointF(10, -10)
                 << QPointF(0, 0)
                 << QPointF(-10, 10) << QPointF(10, 10);
            break;
        case 6:
            dots << QPointF(-10, -10) << QPointF(-10, 0) << QPointF(-10, 10)
                 << QPointF(10, -10) << QPointF(10, 0) << QPointF(10, 10);
            break;
        }

        for (const QPointF &pos : dots)
        {
            painter.drawEllipse(pos, dotSize / 2, dotSize / 2);
        }
    }
    void updateCurrentPlayerIndicator()
    {
        for (auto it = m_playerArrows.begin(); it != m_playerArrows.end(); ++it)
        {
            if (it.value())
            {
                // Create a fade effect for the arrows
                QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(it.value());
                it.value()->setGraphicsEffect(effect);

                QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity");
                animation->setDuration(300); // 300ms transition

                if (it.key() == currentPlayer)
                {
                    it.value()->setVisible(true);
                    animation->setStartValue(0.0);
                    animation->setEndValue(1.0);
                }
                else
                {
                    animation->setStartValue(1.0);
                    animation->setEndValue(0.0);
                    connect(animation, &QPropertyAnimation::finished, [label = it.value()]()
                            { label->setVisible(false); });
                }

                animation->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
        update();
    }
    void moveToSafeZone(Token &token)
    {
        token.inSafeZone = true; // Mark the token as being in the safe zone
        switch (currentPlayer)
        {
        case 0: // Blue's safe zone (6, 1)
            token.x = 1;
            token.y = 6;
            break;
        case 1: // Red's safe zone (1, 6)
            token.x = 8;
            token.y = 1;
            break;
        case 2: // Yellow's safe zone (13, 6)
            token.x = 6;
            token.y = 13;
            break;
        case 3: // Green's safe zone (6, 13)
            token.x = 13;
            token.y = 8;
            break;
        }
    }

    void stopDiceRoll()
    {
        isRolling = false;
        rollTimer->stop();
        diceValue = QRandomGenerator::global()->bounded(1, 7);

        if (diceValue == 6)
        {
            consecutiveSixes++;

            // Save the current state before making any moves
            TurnState currentState;
            currentState.tokenPositions = playerTokens[currentPlayer];
            currentState.diceValue = diceValue;
            turnHistory.push(currentState);

            if (consecutiveSixes == 3)
            {
                revertConsecutiveSixesMoves();
                displayLostTurnMessage();
                consecutiveSixes = 0;
                waitingForMove = false;
                advanceTurn();
                update();
                return;
            }
            bonusRolls++;
        }
        else
        {
            consecutiveSixes = 0;
            // Clear turn history if we didn't roll a 6
            while (!turnHistory.empty())
            {
                turnHistory.pop();
            }
        }

        bool canMove = false;
        for (const Token &token : playerTokens[currentPlayer])
        {
            if (!isTokenInHome(token) || diceValue == 6)
            {
                canMove = true;
                break;
            }
        }

        if (canMove)
        {
            waitingForMove = true;
        }
        else
        {
            consecutiveSixes = 0;
            bonusRolls = 0;
            advanceTurn();
        }

        update();
    }

    void revertConsecutiveSixesMoves()
    {
        while (!turnHistory.empty())
        {
            TurnState previousState = turnHistory.top();
            if (previousState.diceValue == 0)
            {
                // This is the initial state before the first 6
                playerTokens[currentPlayer] = previousState.tokenPositions;
                break;
            }
            turnHistory.pop();
        }
        // Clear the remaining history
        while (!turnHistory.empty())
        {
            turnHistory.pop();
        }
    }

    void saveTurnStartPositions()
    {
        turnStartPositions.clear();
        for (int i = 0; i < playerTokens[currentPlayer].size(); ++i)
        {
            turnStartPositions.push_back({i, playerTokens[currentPlayer][i]});
        }
    }
    void revertAllMovesFromTurn()
    {
        if (!turnStartPositions.empty())
        {
            for (const auto &startPos : turnStartPositions)
            {
                int tokenIndex = startPos.first;
                const Token &startToken = startPos.second;
                playerTokens[currentPlayer][tokenIndex].x = startToken.x;
                playerTokens[currentPlayer][tokenIndex].y = startToken.y;
                playerTokens[currentPlayer][tokenIndex].inSafeZone = startToken.inSafeZone;
            }
        }
        turnStartPositions.clear();
    }
    void revertMoves()
    {
        for (const auto &prevPos : previousPositions)
        {
            int tokenIndex = prevPos.first;
            const Token &prevToken = prevPos.second;
            playerTokens[currentPlayer][tokenIndex].x = prevToken.x;
            playerTokens[currentPlayer][tokenIndex].y = prevToken.y;
            playerTokens[currentPlayer][tokenIndex].inSafeZone = prevToken.inSafeZone;
        }
    }
    void savePreviousPositions()
    {
        previousPositions.clear();
        for (int i = 0; i < playerTokens[currentPlayer].size(); ++i)
        {
            previousPositions.push_back({i, playerTokens[currentPlayer][i]});
        }
    }
    void displayLostTurnMessage()
    {
        QMessageBox::information(this, "Turn Lost",
                                 QString("Player %1 rolled three consecutive sixes and lost their turn!").arg(currentPlayer + 1));
    }
    void endPlayerTurn()
    {
        turnTimer->stop();

        // Reset turn-related variables
        gameDice.value = 0;
        canMoveToSafeZone = false;
        isBonusTurn = false;
        bonusRolls = 0;
        moveValues.clear();
        pendingMoves = 0;
        waitingForMove = false;

        // Advance to next player following clockwise order
        switch (currentPlayer)
        {
        case 0: // Blue -> Red
            currentPlayer = 1;
            break;
        case 1: // Red -> Green
            currentPlayer = 3;
            break;
        case 3: // Green -> Yellow
            currentPlayer = 2;
            break;
        case 2: // Yellow -> Blue
            currentPlayer = 0;
            break;
        }

        // Reset turn timer for new player
        turnTimeLeft = 5;
        turnExpired = false;
        turnTimer->start();

        update();
    }
    void initializePlayers()
    {
        QVector<PlayerInitializer *> initializers;

        // Define colors for players
        QVector<QColor> playerColors = {
            Qt::blue,   // Player 1
            Qt::red,    // Player 2
            Qt::yellow, // Player 3
            Qt::green   // Player 4
        };

        for (int i = 0; i < (m_is2PMode ? 2 : MAX_PLAYERS); ++i)
        {
            PlayerInitializer *initializer = new PlayerInitializer(
                playerTokens,
                i,
                tokensPerPlayer,
                m_tokenType,
                m_is2PMode ? (i == 0 ? m_p1Color : m_p2Color) : playerColors[i]);
            initializers.push_back(initializer);
            initializer->start();
        }

        // Wait for all threads to finish
        for (auto initializer : initializers)
        {
            initializer->wait();
            delete initializer;
        }
    }

    void initialize2PlayerMode()
    {
        playerTokens.clear();
        finishedTokensCount.clear();

        // Create initializers with specific colors
        PlayerInitializer *init1 = new PlayerInitializer(playerTokens, 0, tokensPerPlayer,
                                                         m_tokenType, m_p1Color);
        PlayerInitializer *init2 = new PlayerInitializer(playerTokens, 1, tokensPerPlayer,
                                                         m_tokenType, m_p2Color);

        init1->start();
        init2->start();

        init1->wait();
        init2->wait();

        delete init1;
        delete init2;

        // Initialize finished tokens count
        finishedTokensCount[0] = 0;
        finishedTokensCount[1] = 0;
    }

public:
    void initialize4PlayerMode()
    {
        playerTokens.clear();
        QVector<PlayerInitializer *> initializers;

        // Default colors for 4P mode
        QVector<QColor> playerColors = {
            Qt::blue,   // Player 1
            Qt::red,    // Player 2
            Qt::yellow, // Player 3
            Qt::green   // Player 4
        };

        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            PlayerInitializer *initializer = new PlayerInitializer(
                playerTokens,
                i,
                tokensPerPlayer,
                m_tokenType,
                playerColors[i] // Pass the color for each player
            );
            initializers.push_back(initializer);
            initializer->start();
        }

        for (auto initializer : initializers)
        {
            initializer->wait();
            delete initializer;
        }

        // Initialize finished tokens count
        for (int i = 0; i < MAX_PLAYERS; ++i)
        {
            finishedTokensCount[i] = 0;
        }
    }

private:
    void drawPlayerStats(QPainter &painter)
    {
        painter.save();

        // Set up the font with support for emojis
        QFont emojiFont("Noto Color Emoji", 10, QFont::Bold);
        painter.setFont(emojiFont);

        // Function to create gradient background
        auto drawStatsBackground = [&](const QRectF &rect, const QColor &color)
        {
            QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
            QColor startColor = color.lighter(150);
            QColor endColor = color;
            startColor.setAlpha(180);
            endColor.setAlpha(180);
            gradient.setColorAt(0, startColor);
            gradient.setColorAt(1, endColor);

            // Draw rounded rectangle with gradient
            QPainterPath path;
            path.addRoundedRect(rect, 10, 10);
            painter.fillPath(path, gradient);

            // Add subtle border
            painter.setPen(QPen(color.lighter(130), 2));
            painter.drawPath(path);
        };

        // Function to format stats text
        auto formatStats = [](int winningTokens, int totalTokens, int hits) -> QString
        {
            return QString("🏆 Winning: %1/%2\n⚔️ Hits: %3")
                .arg(winningTokens)
                .arg(totalTokens)
                .arg(hits);
        };

        // Define positions and sizes
        const int padding = 20;
        const int boxWidth = 120;
        const int boxHeight = 60;

        // Player stats positions
        struct StatsPosition
        {
            QRectF rect;
            Qt::Alignment alignment;
            QColor color;
            int playerId;
        };

        QVector<StatsPosition> positions = {
            // Blue player (top left)
            {QRectF(padding - 15, padding - 30, boxWidth, boxHeight),
             Qt::AlignLeft | Qt::AlignVCenter,
             QColor(0, 0, 255, 180),
             0},

            // Red player (top right)
            {QRectF(GRID_SIZE * TILE_SIZE - boxWidth - padding + 10, padding - 25, boxWidth, boxHeight),
             Qt::AlignRight | Qt::AlignVCenter,
             QColor(255, 0, 0, 180),
             1},

            // Yellow player (bottom left)
            {QRectF(padding - 20, GRID_SIZE * TILE_SIZE - boxHeight - padding + 30, boxWidth, boxHeight),
             Qt::AlignLeft | Qt::AlignVCenter,
             QColor(255, 255, 0, 180),
             2},

            // Green player (bottom right)
            {QRectF(GRID_SIZE * TILE_SIZE - boxWidth - padding, GRID_SIZE * TILE_SIZE - boxHeight - padding + 30, boxWidth, boxHeight),
             Qt::AlignRight | Qt::AlignVCenter,
             QColor(0, 255, 0, 180),
             3}};

        // Draw stats for each player
        for (const auto &pos : positions)
        {
            // Draw background
            drawStatsBackground(pos.rect, pos.color);

            // Set text color (white with shadow for better visibility)
            painter.setPen(Qt::black);
            painter.drawText(pos.rect.adjusted(1, 1, 1, 1),
                             pos.alignment,
                             formatStats(playerStats[pos.playerId].finishedTokens,
                                         playerStats[pos.playerId].tokenCount,
                                         playerStats[pos.playerId].hits));
            painter.setPen(Qt::white);
            painter.drawText(pos.rect,
                             pos.alignment,
                             formatStats(playerStats[pos.playerId].finishedTokens,
                                         playerStats[pos.playerId].tokenCount,
                                         playerStats[pos.playerId].hits));
        }

        painter.restore();
    }
    void drawLudoBoard(QPainter &painter)
    {
        QColor redColor(255, 0, 0);
        QColor greenColor(0, 255, 0);
        QColor yellowColor(255, 255, 0);
        QColor blueColor(0, 0, 255);
        QColor whiteColor(255, 255, 255);
        QColor greyColor(192, 192, 192);

        // Create the grid
        for (int row = 0; row < GRID_SIZE; ++row)
        {
            for (int col = 0; col < GRID_SIZE; ++col)
            {
                QRectF tile(col * TILE_SIZE, row * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1);
                painter.fillRect(tile, whiteColor);

                // Define the home areas
                if (row < 6 && col < 6)
                    painter.fillRect(tile, blueColor); // Blue home
                else if (row < 6 && col > 8)
                    painter.fillRect(tile, redColor); // Red home
                else if (row > 8 && col < 6)
                    painter.fillRect(tile, yellowColor); // Yellow home
                else if (row > 8 && col > 8)
                    painter.fillRect(tile, greenColor); // Green home

                // Safe zones (grey areas where tokens can move)
                if ((row == 6 && col == 1) ||          // Blue safe zone near home
                    (row == 1 && col == 8) ||          // Red safe zone near home
                    (row == 13 && col == 6) ||         // Yellow safe zone near home
                    (row == 8 && col == 13))           // Green safe zone near home
                    painter.fillRect(tile, greyColor); // Safe zones
            }
        }

        // Draw colored paths
        for (int i = 1; i < 6; ++i)
        {
            painter.fillRect(QRectF(7 * TILE_SIZE, i * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1), redColor);
            painter.fillRect(QRectF((14 - i) * TILE_SIZE, 7 * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1), greenColor);
            painter.fillRect(QRectF(7 * TILE_SIZE, (14 - i) * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1), yellowColor);
            painter.fillRect(QRectF(i * TILE_SIZE, 7 * TILE_SIZE, TILE_SIZE - 1, TILE_SIZE - 1), blueColor);
        }

        // Draw the center square
        QPolygonF centerSquare;
        centerSquare << QPointF(6 * TILE_SIZE, 6 * TILE_SIZE)
                     << QPointF(9 * TILE_SIZE, 6 * TILE_SIZE)
                     << QPointF(9 * TILE_SIZE, 9 * TILE_SIZE)
                     << QPointF(6 * TILE_SIZE, 9 * TILE_SIZE);
        QPainterPath centerSquarePath;
        centerSquarePath.addPolygon(centerSquare);
        painter.fillPath(centerSquarePath, whiteColor);

        // Draw colored triangles in the center
        drawCenterTriangles(painter);
    }
    void moveTokenAlongPath(Token &token, int steps)
    {
        if (!canMoveToken(token, steps))
        {
            return;
        }
        std::vector<PathCoordinate> *currentPath = getPathForPlayer(currentPlayer);
        if (!currentPath || currentPath->empty())
        {
            return; // Guard against null or empty paths
        }
        int currentIndex = findTokenIndexInPath(token, *currentPath);
        if (currentIndex == -1)
        {
            return; // Token not found in path
        }

        int pathLength = currentPath->size();
        int targetIndex = currentIndex + steps;

        // Check if move is valid
        if (targetIndex >= pathLength)
        {
            if (targetIndex == pathLength - 1)
            {
                targetIndex = pathLength - 1; // Allow exact landing on winning position
            }
            else
            {
                return; // Invalid move - would overshoot
            }
        }
        // Save current state after move if rolling a 6
        if (diceValue == 6)
        {
            TurnState currentState;
            currentState.tokenPositions = playerTokens[currentPlayer];
            currentState.diceValue = diceValue;
            turnHistory.push(currentState);
        }

        // Calculate target position
        if (currentIndex + steps >= pathLength)
        {
            // Check if the token can reach the winning position exactly
            if (currentIndex + steps == pathLength - 1)
            {
                targetIndex = pathLength - 1; // Allow reaching winning position
            }
            else
            {
                return; // Cannot overshoot winning position
            }
        }
        else
        {
            targetIndex = currentIndex + steps;
        }

        int targetX = (*currentPath)[targetIndex].x;
        int targetY = (*currentPath)[targetIndex].y;

        // Create animation path
        QVector<QPointF> animationPath;
        for (int i = currentIndex + 1; i <= targetIndex; ++i)
        {
            const auto &coord = (*currentPath)[i];
            animationPath.append(QPointF(coord.x * TILE_SIZE, coord.y * TILE_SIZE));
        }

        // Check for token capture
        Token *capturedToken = nullptr;
        int capturedPlayer = -1;

        if (!isTokenInSafeZone(targetX, targetY))
        {
            for (int player = 0; player < MAX_PLAYERS; ++player)
            {
                if (player == currentPlayer || !playerExists(player))
                    continue;

                for (Token &opponentToken : playerTokens.at(player))
                { // POTENTIAL PROBLEM HERE
                    if (opponentToken.x == targetX && opponentToken.y == targetY && !opponentToken.isHome)
                    {
                        capturedToken = &opponentToken;
                        capturedPlayer = player;
                        playerHitCounts[currentPlayer]++;
                        break;
                    }
                }
                if (capturedToken)
                    break;
            }
        }

        // Set up animation
        animState.isAnimating = true;
        animState.token = &token;
        animState.path = animationPath;
        animState.currentStep = 0;
        animState.targetX = targetX;
        animState.targetY = targetY;
        animState.capturedToken = capturedToken;
        animState.capturedPlayer = capturedPlayer;

        // Check if token reaches winning position
        if (targetIndex == pathLength - 1)
        {
            finishedTokensCount[currentPlayer]++;        // Increment finished tokens count
            playerStats[currentPlayer].finishedTokens++; // Update player stats

            // Check if all tokens are finished
            if (finishedTokensCount[currentPlayer] == tokensPerPlayer)
            {
                if (!finishedPlayers[currentPlayer])
                {
                    handlePlayerFinish(currentPlayer);
                }
            }
        }
        if (animState.capturedToken && animState.capturedPlayer != -1)
        {
            playerStats[currentPlayer].hits++; // Update player stats
        }
        if (!animState.timer)
        {
            animState.timer = new QTimer(this);
            connect(animState.timer, &QTimer::timeout, this, &LudoBoard::updateAnimation);
        }

        animState.timer->start(100);
    }
    void updateWinningPositions()
    {
        QVector<QPair<int, int>> positions;
        for (const auto &rank : playerRankings)
        {
            positions.append(qMakePair(rank.playerId, rank.rank));
        }
    }
    void drawTokens(QPainter &painter)
    {
        for (const auto &[player, tokens] : playerTokens)
        {
            for (const auto &token : tokens)
            {
                TokenDrawer::drawToken(painter, token, TILE_SIZE);
            }
        }
    }

    void drawCenterTriangles(QPainter &painter)
    {
        QColor redColor(255, 0, 0);
        QColor greenColor(0, 255, 0);
        QColor yellowColor(255, 255, 0);
        QColor blueColor(0, 0, 255);

        // Draw blue triangle
        QPolygonF blueTriangle;
        blueTriangle << QPointF(7.5f * TILE_SIZE, 7.5f * TILE_SIZE)
                     << QPointF(6 * TILE_SIZE, 6 * TILE_SIZE)
                     << QPointF(9 * TILE_SIZE, 6 * TILE_SIZE);
        QPainterPath blueTrianglePath;
        blueTrianglePath.addPolygon(blueTriangle);
        painter.fillPath(blueTrianglePath, redColor);

        QPolygonF redTriangle;
        // Draw red triangle
        redTriangle << QPointF(7.5f * TILE_SIZE, 7.5f * TILE_SIZE)
                    << QPointF(6 * TILE_SIZE, 9 * TILE_SIZE)
                    << QPointF(6 * TILE_SIZE, 6 * TILE_SIZE);
        QPainterPath redTrianglePath;
        redTrianglePath.addPolygon(redTriangle);
        painter.fillPath(redTrianglePath, blueColor);

        // Draw green triangle
        QPolygonF greenTriangle;
        greenTriangle << QPointF(7.5f * TILE_SIZE, 7.5f * TILE_SIZE)
                      << QPointF(9 * TILE_SIZE, 9 * TILE_SIZE)
                      << QPointF(6 * TILE_SIZE, 9 * TILE_SIZE);
        QPainterPath greenTrianglePath;
        greenTrianglePath.addPolygon(greenTriangle);
        painter.fillPath(greenTrianglePath, yellowColor);

        // Draw yellow triangle
        QPolygonF yellowTriangle;
        yellowTriangle << QPointF(7.5f * TILE_SIZE, 7.5f * TILE_SIZE)
                       << QPointF(9 * TILE_SIZE, 6 * TILE_SIZE)
                       << QPointF(9 * TILE_SIZE, 9 * TILE_SIZE);
        QPainterPath yellowTrianglePath;
        yellowTrianglePath.addPolygon(yellowTriangle);
        painter.fillPath(yellowTrianglePath, greenColor);
    }

    void drawTurnIndicator(QPainter &painter)
    {
        int indicatorSize = 60; // Size of the turn indicator

        // Draw a glowing circle around the current player's area
        QColor currentPlayerColor = playerTokens[currentPlayer][0].color; // Get the color of the current player
        painter.setBrush(QBrush(currentPlayerColor));
        painter.setPen(QPen(currentPlayerColor, 3));
        painter.drawEllipse(GRID_SIZE * TILE_SIZE + 100, GRID_SIZE * TILE_SIZE / 2 - 80, indicatorSize, indicatorSize);

        // Optionally, display the current player's name
        QString playerName = (currentPlayer == 0) ? "Blue" : (currentPlayer == 1) ? "Red"
                                                         : (currentPlayer == 2)   ? "Yellow"
                                                                                  : "Green";
        painter.setPen(QPen(Qt::black, 2));
        painter.drawText(GRID_SIZE * TILE_SIZE + 100, GRID_SIZE * TILE_SIZE / 2 - 40, playerName);

        // Display the remaining time for the current player's turn
        painter.setPen(QPen(Qt::black, 2));
        painter.drawText(GRID_SIZE * TILE_SIZE + 100, GRID_SIZE * TILE_SIZE / 2 + 10,
                         QString("Time Left: %1s").arg(turnTimeLeft));
    }

    void advanceTurn()
    {
        int initialPlayer = currentPlayer;
        bool foundActivePlayer = false;
        int attempts = 0;
        int maxAttempts = m_is2PMode ? 2 : 4;

        do
        {
            // Select next player randomly from remaining players
            selectRandomPlayerFromRemaining();
            attempts++;

            // Check if the new current player has any active tokens
            if (playerTokens.find(currentPlayer) != playerTokens.end())
            {
                for (const Token &token : playerTokens[currentPlayer])
                {
                    if (!isWinningPosition(token, currentPlayer))
                    {
                        foundActivePlayer = true;
                        break;
                    }
                }
            }

            // Break if we found an active player or checked all players
            if (foundActivePlayer || attempts >= maxAttempts || currentPlayer == initialPlayer)
            {
                break;
            }
        } while (!foundActivePlayer);

        // If no active players found, trigger game over
        if (!playerExists(currentPlayer))
        {
            checkGameOver();
            return;
        }

        // Update the side panel indicator
        if (m_sidePanel)
        {
            m_sidePanel->setActivePlayer(currentPlayer);
        }

        // Reset turn-related variables
        waitingForMove = false;
        isRolling = false;
        consecutiveSixes = 0;
        turnTimeLeft = 30;
        update();
    }

    void initializeNewIteration()
    {
        remainingPlayersInIteration.clear();
        // Add active players to the remaining list
        if (m_is2PMode)
        {
            remainingPlayersInIteration << 0 << 1;
        }
        else
        {
            remainingPlayersInIteration << 0 << 1 << 2 << 3;
        }
    }

    void selectRandomPlayerFromRemaining()
    {
        if (remainingPlayersInIteration.isEmpty())
        {
            initializeNewIteration(); // Start new iteration when all players have had their turn
        }

        // Randomly select a player from remaining players
        int randomIndex = QRandomGenerator::global()->bounded(remainingPlayersInIteration.size());
        currentPlayer = remainingPlayersInIteration.takeAt(randomIndex);
    }
    bool hasActiveTokens(int player)
    {
        for (const Token &token : playerTokens[player])
        {
            if (!isWinningPosition(token, player))
            {
                return true;
            }
        }
        return false;
    }
    void updateAnimation()
    {
        if (!animState.isAnimating || !animState.token || animState.currentStep >= animState.path.size())
        {
            animState.isAnimating = false;
            if (animState.timer)
            {
                animState.timer->stop();
            }

            // Handle token capture after animation completes
            if (animState.capturedToken && animState.capturedPlayer != -1)
            {
                QString capturedColor;
                switch (animState.capturedPlayer)
                {
                case 0:
                    capturedColor = "Blue";
                    break;
                case 1:
                    capturedColor = "Red";
                    break;
                case 2:
                    capturedColor = "Yellow";
                    break;
                case 3:
                    capturedColor = "Green";
                    break;
                }

                // Move captured token to home
                moveTokenToHome(*animState.capturedToken, animState.capturedPlayer);

                // Show capture message
                QMessageBox::information(this, "Token Captured",
                                         QString("%1's token was captured and sent back home!").arg(capturedColor));

                // Give bonus turn for capturing
                if (diceValue != 6)
                { // Don't give bonus if already got a 6
                    isRolling = false;
                    waitingForMove = false;
                    return; // Don't advance turn, player gets another roll
                }
            }

            // Complete the move
            if (animState.token)
            {
                animState.token->x = animState.targetX;
                animState.token->y = animState.targetY;
            }

            // Handle turn progression
            if (!waitingForMove)
            {
                if (diceValue == 6)
                {
                    isRolling = false;
                    waitingForMove = false;
                }
                else
                {
                    consecutiveSixes = 0;
                    bonusRolls = 0;
                    while (!turnHistory.empty())
                    {
                        turnHistory.pop();
                    }
                    advanceTurn();
                }
            }
            return;
        }

        // Update token position during animation
        QPointF nextPos = animState.path[animState.currentStep];
        animState.token->x = nextPos.x() / TILE_SIZE;
        animState.token->y = nextPos.y() / TILE_SIZE;

        animState.currentStep++;
        update();
    }

    void setupSidePanel()
    {
        m_sidePanel = new SidePanel;
        m_sidePanel->setFixedWidth(300);
        m_sidePanel->setStyleSheet("background-color: #001a33; color: white;");

        QVBoxLayout *panelLayout = new QVBoxLayout(m_sidePanel);
        panelLayout->setSpacing(10);
        panelLayout->setContentsMargins(10, 20, 10, 20);

        // Add player indicators
        setupPlayerIndicators(panelLayout);

        // Add menu buttons
        setupMenuButtons(panelLayout);

        // Add throw dice button
        setupThrowDiceButton(panelLayout);
    }
    void setupPlayerIndicators(QVBoxLayout *layout)
    {
        QWidget *playerContainer = new QWidget;
        QVBoxLayout *playerLayout = new QVBoxLayout(playerContainer);
        playerLayout->setSpacing(4);
        playerLayout->setContentsMargins(4, 4, 4, 4);

        const QVector<QPair<QString, QColor>> players = {
            {"Jinx", Qt::blue},
            {"Ekko", Qt::red},
            {"Vi", Qt::yellow},
            {"Caitlyn", Qt::green}};

        for (const auto &player : players)
        {
            PlayerIndicator *indicator = new PlayerIndicator(player.first, player.second);
            m_playerIndicators.append(indicator);
            playerLayout->addWidget(indicator);
        }

        playerContainer->setStyleSheet(
            "background-color: #002244;"
            "border-radius: 8px;");

        layout->addWidget(playerContainer);
    }

    void setupMenuButtons(QVBoxLayout *layout)
    {
        const std::vector<QString> menuItems = {
            "How to Play", "Sound: On", "Themes", "Reset", "About"};

        for (const auto &item : menuItems)
        {
            QPushButton *btn = new QPushButton(item);
            btn->setStyleSheet(
                "QPushButton {"
                "    background-color: #003366;"
                "    color: white;"
                "    border: none;"
                "    padding: 10px;"
                "    border-radius: 5px;"
                "    font-size: 14px;"
                "}"
                "QPushButton:hover { background-color: #004080; }"
                "QPushButton:pressed { background-color: #002952; }");
            layout->addWidget(btn);
        }

        layout->addStretch();
    }
    void setupThrowDiceButton(QVBoxLayout *layout)
    {
        m_throwDiceBtn = new QPushButton("Throw Dice");
        m_throwDiceBtn->setStyleSheet(
            "QPushButton {"
            "    background-color: #0059b3;"
            "    color: white;"
            "    border: none;"
            "    padding: 15px;"
            "    border-radius: 5px;"
            "    font-size: 16px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #0066cc; }"
            "QPushButton:pressed { background-color: #004d99; }");

        connect(m_throwDiceBtn, &QPushButton::clicked, this, &LudoBoard::startDiceRoll);
        layout->addWidget(m_throwDiceBtn);
    }

    // Property getters and setters for animations
    qreal diceRotation() const { return m_diceRotation; }
    void setDiceRotation(qreal r)
    {
        if (m_diceRotation != r)
        {
            m_diceRotation = r;
            emit diceRotationChanged();
            update();
        }
    }
    QPointF dicePos() const { return m_dicePos; }
    void setDicePos(const QPointF &p)
    {
        if (m_dicePos != p)
        {
            m_dicePos = p;
            emit dicePosChanged();
            update();
        }
    }
    qreal diceScale() const { return m_diceScale; }
    void setDiceScale(qreal s)
    {
        if (m_diceScale != s)
        {
            m_diceScale = s;
            emit diceScaleChanged();
            update();
        }
    }

    bool areAllTokensAtWinningPosition(int playerId)
    {
        for (const Token &token : playerTokens[playerId])
        {
            if (!isWinningPosition(token, playerId))
            {
                return false;
            }
        }
        return true;
    }

    void handlePlayerFinish(int playerId)
    {
        if (finishedPlayers[playerId])
            return;

        finishedPlayers[playerId] = true;
        PlayerRank rank;
        rank.playerId = playerId;
        rank.rank = playerRankings.size() + 1;
        rank.hasFinished = true;
        playerRankings.push_back(rank);

        // Convert rankings to the format expected by SidePanel
        QVector<QPair<int, int>> positions;
        for (const auto &ranking : playerRankings)
        {
            positions.append(qMakePair(ranking.playerId, ranking.rank));
        }

        // Update the side panel with new rankings
        if (m_sidePanel)
        {
            static_cast<SidePanel *>(m_sidePanel)->updateWinningPositions(positions);
        }

        // Show congratulatory message
        QString rankText;
        QString emoji;
        switch (rank.rank)
        {
        case 1:
            rankText = "1st";
            emoji = "🏆";
            break;
        case 2:
            rankText = "2nd";
            emoji = "🥈";
            break;
        case 3:
            rankText = "3rd";
            emoji = "🥉";
            break;
        default:
            rankText = QString("%1th").arg(rank.rank);
            emoji = "🏅";
        }

        QMessageBox::information(this, "Player Finished",
                                 QString("%1 Player %2 finished in %3 place! %4")
                                     .arg(emoji)
                                     .arg(playerId + 1)
                                     .arg(rankText)
                                     .arg(rank.rank == 1 ? "Congratulations!" : "Well played!"));
    }

    void drawPlayerRankings(QPainter &painter)
    {
        for (const PlayerRank &rank : playerRankings)
        {
            QColor playerColor;
            QString playerName;
            int x = GRID_SIZE * TILE_SIZE + 150; // Position on the right side
            int y = 50 + rank.playerId * 60;     // Vertical spacing between indicators

            switch (rank.playerId)
            {
            case 0:
                playerColor = Qt::blue;
                playerName = "Blue";
                break;
            case 1:
                playerColor = Qt::red;
                playerName = "Red";
                break;
            case 2:
                playerColor = Qt::yellow;
                playerName = "Yellow";
                break;
            case 3:
                playerColor = Qt::green;
                playerName = "Green";
                break;
            }

            // Set up font for ranking text
            QFont rankFont = painter.font();
            rankFont.setBold(true);
            rankFont.setPointSize(12);
            painter.setFont(rankFont);
            painter.setPen(Qt::white);
        }
    }

    void drawCrown(QPainter &painter, int x, int y)
    {
        painter.save();
        painter.translate(x, y);

        // Draw a golden crown
        QPainterPath crownPath;

        // Adjusted crown dimensions for better visibility
        const int crownWidth = 20;  // Made slightly smaller
        const int crownHeight = 15; // Made slightly smaller

        // Draw crown points
        crownPath.moveTo(0, crownHeight);
        crownPath.lineTo(crownWidth / 4, crownHeight / 2);
        crownPath.lineTo(crownWidth / 2, 0);
        crownPath.lineTo(crownWidth * 3 / 4, crownHeight / 2);
        crownPath.lineTo(crownWidth, crownHeight);
        crownPath.lineTo(0, crownHeight);

        // Fill crown with gradient
        QLinearGradient gradient(0, 0, 0, crownHeight);
        gradient.setColorAt(0, QColor(255, 215, 0));  // Bright gold at top
        gradient.setColorAt(1, QColor(218, 165, 32)); // Darker gold at bottom

        painter.setBrush(gradient);
        painter.setPen(QPen(Qt::black, 1));
        painter.drawPath(crownPath);

        // Add jewels with adjusted positions
        painter.setBrush(Qt::red);
        painter.drawEllipse(QPointF(crownWidth / 4, crownHeight / 2), 2, 2);
        painter.setBrush(Qt::blue);
        painter.drawEllipse(QPointF(crownWidth / 2, crownHeight / 2), 2, 2);
        painter.setBrush(Qt::green);
        painter.drawEllipse(QPointF(crownWidth * 3 / 4, crownHeight / 2), 2, 2);

        painter.restore();
    }
    void checkGameOver()
    {
        bool gameIsOver = true;
        bool anyPlayerActive = false;
        QVector<QPair<int, QPair<QColor, int>>> playerScores;

        // Check if all tokens are in winning positions or if no more moves are possible
        for (const auto &[playerId, tokens] : playerTokens)
        {
            if (!m_is2PMode || (playerId <= 1))
            { // Only check active players
                bool playerFinished = true;
                bool playerCanMove = false;
                for (const Token &token : tokens)
                {
                    if (!isWinningPosition(token, playerId))
                    {
                        playerFinished = false;
                        if (canTokenMove(token, playerId))
                        {
                            playerCanMove = true;
                            anyPlayerActive = true;
                            break;
                        }
                    }
                }

                if (playerFinished || !playerCanMove)
                {
                    // Calculate score based on position, hits, and remaining tokens
                    int score = (4 - playerRankings.size()) * 1000; // Earlier finish = higher score
                    int hits = playerHitCounts[playerId];           // Track hits in the game
                    int remainingTokens = std::count_if(tokens.begin(), tokens.end(),
                                                        [this, playerId](const Token &t)
                                                        { return !isWinningPosition(t, playerId); });
                    score -= remainingTokens * 100; // Penalty for unfinished tokens
                    playerScores.append({score, {getPlayerColor(playerId), hits}});
                }
                else
                {
                    gameIsOver = false;
                }
            }
        }

        if (gameIsOver || !anyPlayerActive)
        {
            // Sort scores in descending order
            std::sort(playerScores.begin(), playerScores.end(),
                      [](const auto &a, const auto &b)
                      { return a.first > b.first; });
        }
    }
    void drawStarOverlay(QPainter &painter, const QRectF &rect)
    {
        painter.save();
        painter.setPen(QPen(Qt::white, 2));

        QPolygonF star;
        const int points = 5;
        const double outerRadius = rect.width() * 0.35;
        const double innerRadius = rect.width() * 0.15;

        for (int i = 0; i < points * 2; ++i)
        {
            double radius = (i % 2 == 0) ? outerRadius : innerRadius;
            double angle = i * M_PI / points;
            star << QPointF(rect.center().x() + radius * cos(angle),
                            rect.center().y() + radius * sin(angle));
        }
        painter.drawPolygon(star);
        painter.restore();
    }

    void drawLightningOverlay(QPainter &painter, const QRectF &rect)
    {
        painter.save();
        painter.setPen(QPen(Qt::white, 2));

        QPolygonF lightning;
        double w = rect.width() * 0.6;
        double h = rect.height() * 0.7;
        QPointF c = rect.center();

        lightning << QPointF(c.x(), c.y() - h / 2)
                  << QPointF(c.x() - w / 4, c.y())
                  << QPointF(c.x(), c.y())
                  << QPointF(c.x() - w / 4, c.y() + h / 2)
                  << QPointF(c.x() + w / 4, c.y())
                  << QPointF(c.x(), c.y());

        painter.drawPolyline(lightning);
        painter.restore();
    }

    void drawDiamondOverlay(QPainter &painter, const QRectF &rect)
    {
        painter.save();
        painter.setPen(QPen(Qt::white, 2));

        QPolygonF diamond;
        double size = rect.width() * 0.4;
        QPointF c = rect.center();

        diamond << QPointF(c.x(), c.y() - size)
                << QPointF(c.x() + size, c.y())
                << QPointF(c.x(), c.y() + size)
                << QPointF(c.x() - size, c.y())
                << QPointF(c.x(), c.y() - size);

        painter.drawPolygon(diamond);
        painter.restore();
    }

public:
    void resetGame()
    {
        // Reset all game state variables
        playerTokens.clear();
        playerRankings.clear();

        // Reset player stats
        for (auto &[playerId, stats] : playerStats)
        {
            stats.reset();
        }

        currentPlayer = 0;
        consecutiveSixes = 0;
        bonusRolls = 0;
        diceValue = 1;
        waitingForMove = false;
        turnTimeLeft = 30;

        // Reinitialize the game board
        if (m_is2PMode)
        {
            initialize2PlayerMode();
        }
        else
        {
            initialize4PlayerMode();
        }

        // Reset UI elements
        update();
    }
    void initializePlayerStats()
    {
        playerStats.clear();
        int maxPlayers = m_is2PMode ? 2 : MAX_PLAYERS;

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            QColor playerColor = m_is2PMode ? (i == 0 ? m_p1Color : m_p2Color) : getPlayerColor(i);

            playerStats[i] = PlayerStats(tokensPerPlayer, playerColor);
        }
    }
    void updatePlayerStats(int playerId, bool tokenFinished = false, bool hitScored = false)
    {
        if (playerStats.find(playerId) != playerStats.end())
        {
            if (tokenFinished)
            {
                playerStats[playerId].finishedTokens++;
            }
            if (hitScored)
            {
                playerStats[playerId].hits++;
            }
        }
    }

private:
    bool canTokenMove(const Token &token, int playerId)
    {
        // Check if the token can make any valid move
        // This is a simplified version, you may need to adapt it to your game rules
        if (isTokenInHome(token))
        {
            return diceValue == 6;
        }
        else
        {
            // Check if the token can move along its path
            std::vector<PathCoordinate> *path = getPathForPlayer(playerId);
            int currentIndex = findTokenIndexInPath(token, *path);
            return (currentIndex + diceValue) < path->size();
        }
    }
    QColor getPlayerColor(int playerId)
    {
        if (m_is2PMode)
        {
            return playerId == 0 ? m_p1Color : m_p2Color;
        }

        switch (playerId)
        {
        case 0:
            return Qt::blue;
        case 1:
            return Qt::red;
        case 2:
            return Qt::yellow;
        case 3:
            return Qt::green;
        default:
            return Qt::black;
        }
    }

private:
    int tokensPerPlayer;
    QTimer *rollTimer;
    bool isRolling;
    float gameDiceRotation;          // Rotation angle
    QElapsedTimer gameDiceRollClock; // For managing the dice roll time
    QTimer *turnTimer;               // Timer for the player's turn
    int turnTimeLeft = 5;            // Time left for the current player's turn (in seconds)
    bool turnExpired = false;        // Flag to check if the turn expired
    int consecutiveSixes = 0;
    int bonusRolls = 0;
    int diceValue = 1;
    bool isBonusTurn = false;
    bool waitingForMove = false;
    int pendingMoves = 0;
    TokenDrawer tokenDrawer;
    QVector<int> moveValues;
    QStack<TurnState> turnHistory;
    QMap<int, int> playerHitCounts; // Track hits for each player
    std::vector<std::pair<int, Token>> previousPositions;
    std::vector<std::pair<int, Token>> turnStartPositions; // Add this new member variable
    std::map<int, PlayerStats> playerStats;
    std::vector<PathCoordinate> bluePath;
    std::vector<PathCoordinate> redPath;
    std::vector<PathCoordinate> yellowPath;
    std::vector<PathCoordinate> greenPath;
    QVector<PlayerIndicator *> m_playerIndicators;
    std::map<int, int> finishedTokensCount; // Track number of finished tokens per player
    SidePanel *m_sidePanel;
    QPushButton *m_throwDiceBtn;
    int consecutiveTurnsWithoutAction[MAX_PLAYERS] = {0};
    QMap<int, QLabel *> m_playerArrows;
    bool m_is2PMode;
    int m_tokenCount;
    QColor m_p1Color;
    QColor m_p2Color;
    QString m_tokenType;
    // Dice animation properties
    qreal m_diceRotation;
    qreal m_diceScale;
    QPointF m_dicePos;
    bool m_isRolling;
    bool m_diceVisible;
    QVector<int> remainingPlayersInIteration;
    struct AnimationState
    {
        bool isAnimating = false;
        Token *token = nullptr;
        Token *capturedToken = nullptr;
        int capturedPlayer = -1;
        QVector<QPointF> path;
        int currentStep = 0;
        QTimer *timer = nullptr;
        int targetX = 0;
        int targetY = 0;
    } animState;
    void initializePaths()
    {
        // Blue path
        bluePath = {
            {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 5}, {6, 4}, {6, 3}, {6, 2}, {6, 1}, {6, 0}, {7, 0}, {8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5}, {9, 6}, {10, 6}, {11, 6}, {12, 6}, {13, 6}, {14, 6}, {14, 7}, {14, 8}, {13, 8}, {12, 8}, {11, 8}, {10, 8}, {9, 8}, {8, 9}, {8, 10}, {8, 11}, {8, 12}, {8, 13}, {8, 14}, {7, 14}, {6, 14}, {6, 13}, {6, 12}, {6, 11}, {6, 10}, {6, 9}, {5, 8}, {4, 8}, {3, 8}, {2, 8}, {1, 8}, {0, 8}, {0, 7},
            // Blue's colored path
            {1, 7},
            {2, 7},
            {3, 7},
            {4, 7},
            {5, 7},
            {6, 7}}; // winning coordinate

        // Red path
        redPath = {
            {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5}, {9, 6}, {10, 6}, {11, 6}, {12, 6}, {13, 6}, {14, 6}, {14, 7}, {14, 8}, {13, 8}, {12, 8}, {11, 8}, {10, 8}, {9, 8}, {8, 9}, {8, 10}, {8, 11}, {8, 12}, {8, 13}, {8, 14}, {7, 14}, {6, 14}, {6, 13}, {6, 12}, {6, 11}, {6, 10}, {6, 9}, {5, 8}, {4, 8}, {3, 8}, {2, 8}, {1, 8}, {0, 8}, {0, 7}, {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6}, {6, 5}, {6, 4}, {6, 3}, {6, 2}, {6, 1}, {6, 0}, {7, 0},
            // Red's colored path
            {7, 1},
            {7, 2},
            {7, 3},
            {7, 4},
            {7, 5},
            {7, 6}}; // winning coordinate

        // Yellow path
        yellowPath = {
            {6, 13},
            {6, 12},
            {6, 11},
            {6, 10},
            {6, 9},
            {5, 8},
            {4, 8},
            {3, 8},
            {2, 8},
            {1, 8},
            {0, 8},
            {0, 7},
            {0, 6},
            {1, 6}, // Blue exit
            {2, 6},
            {3, 6},
            {4, 6},
            {5, 6},
            {6, 5},
            {6, 4},
            {6, 3},
            {6, 2},
            {6, 1},
            {6, 0},
            {7, 0},
            {8, 0},
            {8, 1},
            {8, 2},
            {8, 3},
            {8, 4},
            {8, 5},
            {9, 6},
            {10, 6},
            {11, 6},
            {12, 6},
            {13, 6},
            {14, 6},
            {14, 7},
            {14, 8},
            {13, 8}, // Green exit
            {12, 8},
            {11, 8},
            {10, 8},
            {9, 8},
            {8, 9},
            {8, 10},
            {8, 11},
            {8, 12},
            {8, 13},
            {8, 14},
            // Yellow's colored path
            {7, 14},
            {7, 13},
            {7, 12},
            {7, 11},
            {7, 10},
            {7, 9},
            {7, 8}}; // winning coordinate

        // Green path
        greenPath = {
            {13, 8}, // Green exit
            {12, 8},
            {11, 8},
            {10, 8},
            {9, 8},
            {8, 9},
            {8, 10},
            {8, 11},
            {8, 12},
            {8, 13},
            {8, 14},
            {7, 14}, // Yellow exit
            {6, 14},
            {6, 13},
            {6, 12},
            {6, 11},
            {6, 10},
            {6, 9},
            {5, 8},
            {4, 8},
            {3, 8},
            {2, 8},
            {1, 8},
            {0, 8},
            {0, 7},
            {0, 6},
            {1, 6}, // Blue exit
            {2, 6},
            {3, 6},
            {4, 6},
            {5, 6},
            {6, 5},
            {6, 4},
            {6, 3},
            {6, 2},
            {6, 1},
            {6, 0},
            {7, 0},
            {8, 0},
            {8, 1},
            {8, 2},
            {8, 3},
            {8, 4},
            {8, 5},
            {9, 6},
            {10, 6},
            {11, 6},
            {12, 6},
            {13, 6},
            {14, 6},
            {14, 7},
            // Green's colored path
            {14, 7},
            {13, 7},
            {12, 7},
            {11, 7},
            {10, 7},
            {9, 7},
            {8, 7}}; // winning coordinate
    }
};
class AnimatedButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit AnimatedButton(const QString &text, const QColor &color, QWidget *parent = nullptr)
        : QPushButton(text, parent), m_scale(1.0), m_opacity(1.0), m_baseColor(color)
    {

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setFixedSize(320, 65);
        setCursor(Qt::PointingHandCursor);

        // Create shadow effect
        auto *shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(20);
        shadowEffect->setColor(color.darker(150));
        shadowEffect->setOffset(0, 4);
        setGraphicsEffect(shadowEffect);

        updateStyle();
    }

    qreal scale() const { return m_scale; }
    void setScale(qreal scale)
    {
        m_scale = scale;
        updateStyle();
        update();
    }

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal opacity)
    {
        m_opacity = opacity;
        setStyleSheet(QString(
                          "AnimatedButton {"
                          "    color: white;"
                          "    border: none;"
                          "    opacity: %1;"
                          "}")
                          .arg(m_opacity));
        update();
    }

protected:
    void enterEvent(QEvent *event) override
    {
        QPushButton::enterEvent(event);
        animate(1.05, 1.0);
    }

    void leaveEvent(QEvent *event) override
    {
        QPushButton::leaveEvent(event);
        animate(1.0, 1.0);
    }

    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Apply scale transformation
        painter.translate(width() / 2, height() / 2);
        painter.scale(m_scale, m_scale);
        painter.translate(-width() / 2, -height() / 2);

        // Create gradient background
        QLinearGradient gradient(0, 0, width(), 0);
        gradient.setColorAt(0.0, m_baseColor);
        gradient.setColorAt(1.0, m_baseColor.lighter(130));

        // Draw rounded rectangle with gradient
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRoundedRect(rect(), 15, 15);

        // Draw text
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 14, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, text());
    }

private:
    void updateStyle()
    {
        // Handle scaling through paintEvent instead of QWidget::setTransform
        update();
    }

    void animate(qreal targetScale, qreal targetOpacity)
    {
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

        QPropertyAnimation *scaleAnim = new QPropertyAnimation(this, "scale");
        scaleAnim->setDuration(200);
        scaleAnim->setEasingCurve(QEasingCurve::OutCubic);
        scaleAnim->setStartValue(m_scale);
        scaleAnim->setEndValue(targetScale);
        group->addAnimation(scaleAnim);

        QPropertyAnimation *opacityAnim = new QPropertyAnimation(this, "opacity");
        opacityAnim->setDuration(200);
        opacityAnim->setStartValue(m_opacity);
        opacityAnim->setEndValue(targetOpacity);
        group->addAnimation(opacityAnim);

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

    qreal m_scale;
    qreal m_opacity;
    QColor m_baseColor;
};

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);

signals:
    void gameStarted(bool is2PMode);

private slots:
    void showPlayerSelection();
    void animateButtons();

private:
    void setupUI();
    void createTitle();
    void createButtons();
    void startButtonAnimations();

    QVBoxLayout *m_mainLayout;
    QLabel *m_titleLabel;
    QVector<AnimatedButton *> m_buttons;
    int m_currentButtonIndex;
    QTimer *m_animationTimer;
};

class GameWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GameWindow(QWidget *parent = nullptr) : QWidget(parent)
    {
        stack = new QStackedWidget(this);
        mainMenu = new MainMenu;
        ludoBoard = nullptr;

        stack->addWidget(mainMenu);

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(stack);

        connect(mainMenu, &MainMenu::gameStarted, this, &GameWindow::startGame);

        setFixedSize(800, 600);
        stack->setCurrentWidget(mainMenu);
    }

private slots:
    void startGame(bool is2PMode)
    {
        if (ludoBoard)
        {
            stack->removeWidget(ludoBoard);
            delete ludoBoard;
        }

        // Get the token type and quantity from the dialog
        TokenSelectionDialog dialog(is2PMode, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            int selectedTokenCount = dialog.getTokenCount();
            QString selectedTokenType = dialog.getTokenType();

            QColor p1Color, p2Color, p3Color, p4Color;
            if (is2PMode)
            {
                p1Color = dialog.getPlayer1Color();
                p2Color = dialog.getPlayer2Color();
                p3Color = Qt::yellow; // Default colors for unused players
                p4Color = Qt::green;
            }
            else
            {
                // Fixed colors for 4P mode
                p1Color = Qt::blue;
                p2Color = Qt::red;
                p3Color = Qt::yellow;
                p4Color = Qt::green;
            }

            // Create new board with selected options
            ludoBoard = new LudoBoard(is2PMode, selectedTokenCount, p1Color, p2Color, selectedTokenType);

            // Initialize appropriate mode
            if (!is2PMode)
            {
                ludoBoard->initialize4PlayerMode();
            }

            stack->addWidget(ludoBoard);
            stack->setCurrentWidget(ludoBoard);

            connect(ludoBoard->getSidePanel(), &SidePanel::mainMenuRequested, this, &GameWindow::showMainMenu);
            connect(ludoBoard->getSidePanel(), &SidePanel::restartRequested, this, &GameWindow::restartGame);
            connect(ludoBoard->getSidePanel(), &SidePanel::quitRequested, this, &GameWindow::quitGame);

            setFixedSize(GRID_SIZE * TILE_SIZE + 400, GRID_SIZE * TILE_SIZE);
        }
        else
        {
            stack->setCurrentWidget(mainMenu);
            setFixedSize(800, 600);
        }
    }

    void showMainMenu()
    {
        if (QMessageBox::question(this, "Return to Main Menu",
                                  "Are you sure you want to return to the main menu? Current game progress will be lost.",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            stack->setCurrentWidget(mainMenu);
            setFixedSize(800, 600);
        }
    }

    void restartGame()
    {
        if (QMessageBox::question(this, "Restart Game",
                                  "Are you sure you want to restart the game? Current game progress will be lost.",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            if (ludoBoard)
            {
                ludoBoard->resetGame();
            }
        }
    }

    void quitGame()
    {
        if (QMessageBox::question(this, "Quit Game",
                                  "Are you sure you want to quit the game?",
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            QApplication::quit();
        }
    }

private:
    MainMenu *mainMenu;
    LudoBoard *ludoBoard;
    QStackedWidget *stack;
};
MainMenu::MainMenu(QWidget *parent) : QWidget(parent), m_currentButtonIndex(0)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "MainMenu {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "                                stop:0 #2A0944,"
        "                                stop:0.5 #3B185F,"
        "                                stop:1 #A12568);"
        "}");

    setupUI();
    startButtonAnimations();
}

void MainMenu::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(25);
    m_mainLayout->setContentsMargins(40, 60, 40, 60);
    m_mainLayout->setAlignment(Qt::AlignCenter);

    createTitle();
    createButtons();
}

void MainMenu::createTitle()
{
    m_titleLabel = new QLabel("LUDO MASTER", this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "    color: #FFD700;"
        "    font-size: 64px;"
        "    font-weight: bold;"
        "    font-family: 'Arial Black';"
        "    letter-spacing: 2px;"
        "    margin-bottom: 40px;"
        "}");

    // Add glow effect to title
    auto *glowEffect = new QGraphicsDropShadowEffect(this);
    glowEffect->setBlurRadius(30);
    glowEffect->setColor(QColor(255, 215, 0, 160));
    glowEffect->setOffset(0, 0);
    m_titleLabel->setGraphicsEffect(glowEffect);

    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addSpacing(30);
}

void MainMenu::createButtons()
{
    struct ButtonInfo
    {
        QString text;
        QColor color;
    };

    QVector<ButtonInfo> buttonInfos = {
        {"PLAY WITH COMPUTER", QColor("#E74C3C")},
        {"QUICK PLAY", QColor("#3498DB")},
        {"PLAY WITH FRIENDS", QColor("#2ECC71")},
        {"ONLINE MULTIPLAYER", QColor("#F39C12")}};

    for (const auto &info : buttonInfos)
    {
        auto *button = new AnimatedButton(info.text, info.color, this);
        button->setOpacity(0.0); // Start invisible for animation
        m_buttons.append(button);
        m_mainLayout->addWidget(button, 0, Qt::AlignCenter);
    }

    // Connect buttons
    connect(m_buttons[0], &QPushButton::clicked, this, &MainMenu::showPlayerSelection);
    connect(m_buttons[1], &QPushButton::clicked, this, &MainMenu::showPlayerSelection);
}

void MainMenu::startButtonAnimations()
{
    m_animationTimer = new QTimer(this);
    m_currentButtonIndex = 0;
    connect(m_animationTimer, &QTimer::timeout, this, &MainMenu::animateButtons);
    m_animationTimer->start(200);
}

void MainMenu::animateButtons()
{
    if (m_currentButtonIndex >= m_buttons.size())
    {
        m_animationTimer->stop();
        return;
    }

    auto *button = m_buttons[m_currentButtonIndex];
    QPropertyAnimation *animation = new QPropertyAnimation(button, "opacity");
    animation->setDuration(300);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    m_currentButtonIndex++;
}

void MainMenu::showPlayerSelection()
{
    PlayerSelectionDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        emit gameStarted(dialog.getPlayerCount() == 2);
    }
}
int main(int argc, char *argv[])
{
    qInstallMessageHandler(0);
    QApplication a(argc, argv);
    LudoGame L;
    GameWindow w;
    w.setWindowTitle("Ludo Game");
    w.resize(GRID_SIZE * TILE_SIZE + 200, GRID_SIZE * TILE_SIZE);
    w.show();
    return a.exec();
}
#include "main.moc"