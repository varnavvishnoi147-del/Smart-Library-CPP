#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QStringList>
#include <QTimer>
#include <QDateTime>
#include <QQueue>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    static const int TOTAL_SEATS = 500;

    bool booked[TOTAL_SEATS + 1];
    QString names[TOTAL_SEATS + 1];
    QString mobiles[TOTAL_SEATS + 1];
    QString userTypes[TOTAL_SEATS + 1];
    int durations[TOTAL_SEATS + 1];

    // Booking start and expiry times
    QDateTime bookingStart[TOTAL_SEATS + 1];
    QDateTime bookingExpiry[TOTAL_SEATS + 1];

    QPushButton *seatButtons[TOTAL_SEATS + 1];

    QLabel *availableLabel;
    QLabel *occupiedLabel;
    QLabel *totalBookingsLabel;
    QLabel *studentBookingsLabel;
    QLabel *facultyBookingsLabel;
    QLabel *cancellationsLabel;

    QLineEdit *seatInput;
    QLineEdit *nameInput;
    QLineEdit *mobileInput;

    QComboBox *userTypeInput;
    QSpinBox *durationInput;

    struct WaitingStudent
    {
        QString name;
        QString mobile;
        QString userType;
        int duration;
    };
    QQueue<WaitingStudent> waitingList;

    void joinWaitingList();
    void assignNextStudent(int seat);
    void updateWaitingList();

    int selectedSeat;
    int totalBookings;
    int studentBookings;
    int facultyBookings;
    int totalCancellations;

    QStringList bookingHistory;

    QTimer *expiryTimer;

    void setupUI();
    void updateSeats();

    void saveBookings();
    void loadBookings();

    void saveStatistics();
    void loadStatistics();

    void saveBookingHistory();
    void loadBookingHistory();
    void showBookingHistory();

    void selectSeat(int seat);

    void bookSeat();
    void cancelBooking();
    void searchBooking();

    void checkExpiredBookings();
    void expireSeat(int seat);
    void recordHistory(
        const QString &status,
        int seat
        );
};

#endif