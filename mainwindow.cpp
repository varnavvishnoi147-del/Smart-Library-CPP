#include "mainwindow.h"

#include <QWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QCoreApplication>
#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QScrollArea>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    selectedSeat(0),
    totalBookings(0),
    studentBookings(0),
    facultyBookings(0),
    totalCancellations(0),
    expiryTimer(new QTimer(this))
{
    for (int i = 1; i <= TOTAL_SEATS; i++) {
        booked[i] = false;
        names[i] = "";
        mobiles[i] = "";
        userTypes[i] = "";
        durations[i] = 0;
        bookingStart[i] = QDateTime();
        bookingExpiry[i] = QDateTime();
        seatButtons[i] = nullptr;
    }

    setupUI();

    loadBookings();
    loadStatistics();
    loadBookingHistory();

    checkExpiredBookings();
    updateSeats();

    connect(
        expiryTimer,
        &QTimer::timeout,
        this,
        &MainWindow::checkExpiredBookings
        );

    expiryTimer->start(1000);
}

MainWindow::~MainWindow()
{
    saveBookings();
    saveStatistics();
    saveBookingHistory();
}

void MainWindow::setupUI()
{
    setWindowTitle("Smart Library Seat Management System");
    resize(1150, 800);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    central->setStyleSheet(
        "QWidget { background-color: #f3f6fb; color: #263238; }"
        "QLineEdit, QComboBox, QSpinBox {"
        "background: white; padding: 7px;"
        "border: 1px solid #ccd5e0; border-radius: 5px; }"
        "QGroupBox { border: 1px solid #ccd5e0;"
        "border-radius: 8px; margin-top: 12px;"
        "padding: 10px; font-weight: bold; }"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);

    QLabel *title = new QLabel("SMART LIBRARY");
    title->setAlignment(Qt::AlignCenter);
    title->setMinimumHeight(55);

    title->setStyleSheet(
        "background-color: #10265c; color: white;"
        "font-size: 24px; font-weight: bold;"
        "border-radius: 8px;"
        );

    mainLayout->addWidget(title);

    QLabel *subtitle =
        new QLabel("Seat Management & Reservation System");

    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(
        "font-size: 15px; color: #52627a;"
        );

    mainLayout->addWidget(subtitle);

    // Available and occupied counters

    QHBoxLayout *counterLayout = new QHBoxLayout;

    availableLabel = new QLabel("Available: 500");
    occupiedLabel = new QLabel("Occupied: 0");

    availableLabel->setStyleSheet(
        "background: #bcefe5; padding: 10px;"
        "font-size: 15px; font-weight: bold;"
        "border-radius: 6px;"
        );

    occupiedLabel->setStyleSheet(
        "background: #f4d7e5; padding: 10px;"
        "font-size: 15px; font-weight: bold;"
        "border-radius: 6px;"
        );

    counterLayout->addWidget(availableLabel);
    counterLayout->addWidget(occupiedLabel);

    mainLayout->addLayout(counterLayout);

    // Statistics dashboard

    QGroupBox *statisticsBox =
        new QGroupBox("BOOKING STATISTICS");

    QGridLayout *statisticsLayout =
        new QGridLayout(statisticsBox);

    totalBookingsLabel = new QLabel("Total Bookings: 0");
    studentBookingsLabel = new QLabel("Student Bookings: 0");
    facultyBookingsLabel = new QLabel("Faculty Bookings: 0");
    cancellationsLabel = new QLabel("Total Cancellations: 0");

    QLabel *statLabels[] = {
        totalBookingsLabel,
        studentBookingsLabel,
        facultyBookingsLabel,
        cancellationsLabel
    };

    for (QLabel *label : statLabels) {
        label->setMinimumHeight(32);
        label->setStyleSheet(
            "background: white; padding: 7px;"
            "font-size: 13px; border-radius: 5px;"
            );
    }

    statisticsLayout->addWidget(totalBookingsLabel, 0, 0);
    statisticsLayout->addWidget(studentBookingsLabel, 0, 1);
    statisticsLayout->addWidget(facultyBookingsLabel, 1, 0);
    statisticsLayout->addWidget(cancellationsLabel, 1, 1);

    mainLayout->addWidget(statisticsBox);

    // Seat Grid with Scroll Area

    QHBoxLayout *contentLayout = new QHBoxLayout;

    QGroupBox *seatsBox = new QGroupBox("LIBRARY SEATS");

    QVBoxLayout *seatsMainLayout = new QVBoxLayout(seatsBox);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(400);

    QWidget *seatContainer = new QWidget;

    QGridLayout *seatLayout = new QGridLayout(seatContainer);
    seatLayout->setSpacing(8);

    for (int i = 1; i <= TOTAL_SEATS; i++) {

        seatButtons[i] = new QPushButton(QString::number(i));

        seatButtons[i]->setMinimumSize(65, 38);

        seatLayout->addWidget(
            seatButtons[i],
            (i - 1) / 10,
            (i - 1) % 10
            );

        connect(
            seatButtons[i],
            &QPushButton::clicked,
            this,
            [this, i]() {
                selectSeat(i);
            }
            );
    }

    scrollArea->setWidget(seatContainer);

    seatsMainLayout->addWidget(scrollArea);

    contentLayout->addWidget(seatsBox, 3);

    // Booking form

    QGroupBox *bookingBox = new QGroupBox("BOOK YOUR SEAT");
    QVBoxLayout *bookingLayout = new QVBoxLayout(bookingBox);
    QFormLayout *formLayout = new QFormLayout;

    seatInput = new QLineEdit;
    seatInput->setPlaceholderText("Select a seat");
    seatInput->setReadOnly(true);

    nameInput = new QLineEdit;
    nameInput->setPlaceholderText("Enter your name");

    mobileInput = new QLineEdit;
    mobileInput->setPlaceholderText("Enter mobile number");
    mobileInput->setMaxLength(10);

    userTypeInput = new QComboBox;
    userTypeInput->addItems({"Student", "Faculty"});

    durationInput = new QSpinBox;
    durationInput->setRange(1, 12);
    durationInput->setValue(2);
    durationInput->setSuffix(" hours");

    formLayout->addRow("Selected Seat", seatInput);
    formLayout->addRow("Full Name", nameInput);
    formLayout->addRow("Mobile Number", mobileInput);
    formLayout->addRow("User Type", userTypeInput);
    formLayout->addRow("Duration", durationInput);

    bookingLayout->addLayout(formLayout);

    QPushButton *bookButton = new QPushButton("BOOK SEAT");
    QPushButton *cancelButton = new QPushButton("CANCEL BOOKING");
    QPushButton *searchButton = new QPushButton("SEARCH BOOKING");
    QPushButton *historyButton = new QPushButton("BOOKING HISTORY");
    QPushButton *waitingButton =
        new QPushButton("JOIN WAITING LIST");

    waitingButton->setMinimumHeight(38);

    bookButton->setMinimumHeight(38);
    cancelButton->setMinimumHeight(38);
    searchButton->setMinimumHeight(35);
    historyButton->setMinimumHeight(38);

    bookButton->setStyleSheet(
        "QPushButton { background: #0875e1; color: white;"
        "font-weight: bold; border-radius: 6px; padding: 8px; }"
        "QPushButton:hover { background: #075bb0; }"
        );

    cancelButton->setStyleSheet(
        "QPushButton { background: #d93657; color: white;"
        "font-weight: bold; border-radius: 6px; padding: 8px; }"
        "QPushButton:hover { background: #b82340; }"
        );

    searchButton->setStyleSheet(
        "QPushButton { background: #e4eaf2;"
        "border-radius: 6px; padding: 8px; }"
        );

    historyButton->setStyleSheet(
        "QPushButton { background: #183153; color: white;"
        "font-weight: bold; border-radius: 6px; padding: 9px; }"
        );

    bookingLayout->addWidget(bookButton);
    bookingLayout->addWidget(cancelButton);
    bookingLayout->addWidget(searchButton);
    bookingLayout->addWidget(historyButton);
    bookingLayout->addWidget(waitingButton);
    bookingLayout->addStretch();

    contentLayout->addWidget(bookingBox, 2);
    mainLayout->addLayout(contentLayout);

    QLabel *legend = new QLabel(
        "GREEN = Available     RED = Occupied"
        );

    legend->setStyleSheet(
        "color: #52627a; padding: 5px;"
        );

    mainLayout->addWidget(legend);

    connect(
        bookButton, &QPushButton::clicked,
        this, &MainWindow::bookSeat
        );

    connect(
        cancelButton, &QPushButton::clicked,
        this, &MainWindow::cancelBooking
        );

    connect(
        searchButton, &QPushButton::clicked,
        this, &MainWindow::searchBooking
        );

    connect(
        historyButton, &QPushButton::clicked,
        this, &MainWindow::showBookingHistory
        );

    connect(
        waitingButton,
        &QPushButton::clicked,
        this,
        &MainWindow::joinWaitingList
        );
}

void MainWindow::selectSeat(int seat)
{
    selectedSeat = seat;
    seatInput->setText(QString::number(seat));

    if (booked[seat]) {
        QMessageBox::information(
            this,
            "Seat Information",
            "Seat " + QString::number(seat) +
                " is occupied.\nName: " + names[seat] +
                "\nUser Type: " + userTypes[seat]
            );
    }
}

void MainWindow::updateSeats()
{
    int available = 0;
    int occupied = 0;

    for (int i = 1; i <= TOTAL_SEATS; i++) {
        if (booked[i]) {
            occupied++;

            seatButtons[i]->setStyleSheet(
                "QPushButton { background: #e94b67;"
                "color: white; font-weight: bold;"
                "border-radius: 6px; }"
                );
        } else {
            available++;

            seatButtons[i]->setStyleSheet(
                "QPushButton { background: #32ad79;"
                "color: white; font-weight: bold;"
                "border-radius: 6px; }"
                );
        }
    }

    availableLabel->setText(
        "Available: " + QString::number(available)
        );

    occupiedLabel->setText(
        "Occupied: " + QString::number(occupied)
        );

    totalBookingsLabel->setText(
        "Total Bookings: " + QString::number(totalBookings)
        );

    studentBookingsLabel->setText(
        "Student Bookings: " + QString::number(studentBookings)
        );

    facultyBookingsLabel->setText(
        "Faculty Bookings: " + QString::number(facultyBookings)
        );

    cancellationsLabel->setText(
        "Total Cancellations: " +
        QString::number(totalCancellations)
        );
}

void MainWindow::bookSeat()
{
    bool ok = false;
    int seat = seatInput->text().toInt(&ok);

    if (!ok || seat < 1 || seat > TOTAL_SEATS) {
        QMessageBox::warning(
            this, "Invalid Seat",
            "Please select a valid seat."
            );
        return;
    }

    if (booked[seat]) {
        QMessageBox::warning(
            this, "Seat Occupied",
            "This seat is already booked."
            );
        return;
    }

    QString name = nameInput->text().trimmed();
    QString mobile = mobileInput->text().trimmed();
    QString userType = userTypeInput->currentText();
    int duration = durationInput->value();

    if (name.isEmpty()) {
        QMessageBox::warning(
            this, "Missing Name",
            "Please enter your name."
            );
        return;
    }

    QRegularExpression pattern("^[0-9]{10}$");

    if (!pattern.match(mobile).hasMatch()) {
        QMessageBox::warning(
            this, "Invalid Mobile",
            "Enter a valid 10-digit mobile number."
            );
        return;
    }

    QDateTime start = QDateTime::currentDateTime();
    QDateTime expiry = start.addSecs(duration * 3600);

    booked[seat] = true;
    names[seat] = name;
    mobiles[seat] = mobile;
    userTypes[seat] = userType;
    durations[seat] = duration;
    bookingStart[seat] = start;
    bookingExpiry[seat] = expiry;

    totalBookings++;

    if (userType == "Student") {
        studentBookings++;
    } else {
        facultyBookings++;
    }

    recordHistory("BOOKED", seat);

    saveBookings();
    saveStatistics();
    saveBookingHistory();

    updateSeats();

    QMessageBox::information(
        this,
        "Booking Successful",
        "Seat booked successfully!\n"
        "Seat: " + QString::number(seat) +
            "\nName: " + name +
            "\nDuration: " + QString::number(duration) + " hours" +
            "\nExpires at: " + expiry.toString("hh:mm:ss")
        );

    nameInput->clear();
    mobileInput->clear();
}
void MainWindow::cancelBooking()
{
    bool ok = false;
    int seat = seatInput->text().toInt(&ok);

    if (!ok || seat < 1 || seat > TOTAL_SEATS) {
        QMessageBox::warning(
            this, "Invalid Seat",
            "Please select a valid seat."
            );
        return;
    }

    if (!booked[seat]) {
        QMessageBox::information(
            this, "No Booking",
            "This seat is not currently booked."
            );
        return;
    }

    QMessageBox::StandardButton answer =
        QMessageBox::question(
            this,
            "Cancel Booking",
            "Cancel booking for seat " +
                QString::number(seat) + "?",
            QMessageBox::Yes | QMessageBox::No
            );

    if (answer != QMessageBox::Yes) {
        return;
    }

    recordHistory("CANCELLED", seat);

    booked[seat] = false;
    names[seat].clear();
    mobiles[seat].clear();
    userTypes[seat].clear();
    durations[seat] = 0;
    bookingStart[seat] = QDateTime();
    bookingExpiry[seat] = QDateTime();

    totalCancellations++;

    // Give the released seat to the next person in the queue.
    assignNextStudent(seat);

    saveBookings();
    saveStatistics();
    saveBookingHistory();

    updateSeats();

    nameInput->clear();
    mobileInput->clear();

    QMessageBox::information(
        this, "Cancelled",
        "Booking cancelled successfully."
        );
}

void MainWindow::searchBooking()
{
    bool ok = false;
    int seat = seatInput->text().toInt(&ok);

    if (!ok || seat < 1 || seat > TOTAL_SEATS) {
        QMessageBox::warning(
            this, "Invalid Seat",
            "Please select a seat first."
            );
        return;
    }

    if (!booked[seat]) {
        QMessageBox::information(
            this, "Search Result",
            "No active booking found for this seat."
            );
        return;
    }

    qint64 remaining =
        QDateTime::currentDateTime().secsTo(bookingExpiry[seat]);

    if (remaining < 0) {
        remaining = 0;
    }

    QMessageBox::information(
        this, "Booking Details",
        "Seat: " + QString::number(seat) +
            "\nName: " + names[seat] +
            "\nMobile: " + mobiles[seat] +
            "\nUser Type: " + userTypes[seat] +
            "\nDuration: " + QString::number(durations[seat]) + " hours" +
            "\nRemaining seconds: " + QString::number(remaining)
        );
}

void MainWindow::checkExpiredBookings()
{
    bool changed = false;
    QDateTime now = QDateTime::currentDateTime();

    for (int seat = 1; seat <= TOTAL_SEATS; seat++) {

        if (booked[seat] &&
            bookingExpiry[seat].isValid() &&
            now >= bookingExpiry[seat]) {

            expireSeat(seat);
            changed = true;
        }
    }

    if (changed) {
        saveBookings();
        saveStatistics();
        saveBookingHistory();
        updateSeats();
    }
}

void MainWindow::expireSeat(int seat)
{
    recordHistory("EXPIRED", seat);

    booked[seat] = false;
    names[seat].clear();
    mobiles[seat].clear();
    userTypes[seat].clear();
    durations[seat] = 0;
    bookingStart[seat] = QDateTime();
    bookingExpiry[seat] = QDateTime();

    // Assign the expired seat to the next waiting student.
    assignNextStudent(seat);
}

void MainWindow::joinWaitingList()
{
    int occupied = 0;

    for (int i = 1; i <= TOTAL_SEATS; i++) {
        if (booked[i]) {
            occupied++;
        }
    }

    if (occupied < TOTAL_SEATS) {
        QMessageBox::information(
            this,
            "Seats Available",
            "Seats are still available. Please book a seat directly."
            );
        return;
    }

    bool ok = false;

    QString name = QInputDialog::getText(
        this,
        "Waiting List",
        "Enter your full name:",
        QLineEdit::Normal,
        "",
        &ok
        );

    if (!ok) {
        return;
    }

    name = name.trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(
            this,
            "Invalid Name",
            "Please enter your name."
            );
        return;
    }

    QString mobile = QInputDialog::getText(
        this,
        "Waiting List",
        "Enter your 10-digit mobile number:",
        QLineEdit::Normal,
        "",
        &ok
        );

    if (!ok) {
        return;
    }

    mobile = mobile.trimmed();

    QRegularExpression pattern("^[0-9]{10}$");

    if (!pattern.match(mobile).hasMatch()) {
        QMessageBox::warning(
            this,
            "Invalid Mobile",
            "Enter a valid 10-digit mobile number."
            );
        return;
    }

    QStringList types;
    types << "Student" << "Faculty";

    QString userType = QInputDialog::getItem(
        this,
        "Waiting List",
        "Select user type:",
        types,
        0,
        false,
        &ok
        );

    if (!ok) {
        return;
    }

    int duration = QInputDialog::getInt(
        this,
        "Waiting List",
        "Enter booking duration in hours:",
        2,
        1,
        12,
        1,
        &ok
        );

    if (!ok) {
        return;
    }

    WaitingStudent student;

    student.name = name;
    student.mobile = mobile;
    student.userType = userType;
    student.duration = duration;

    waitingList.enqueue(student);

    QMessageBox::information(
        this,
        "Waiting List",
        "You have been added to the waiting list.\n"
        "Your position is: " +
            QString::number(waitingList.size())
        );
}

void MainWindow::assignNextStudent(int seat)
{
    if (waitingList.isEmpty()) {
        return;
    }

    if (seat < 1 || seat > TOTAL_SEATS || booked[seat]) {
        return;
    }

    // FIFO: The first student who joined gets the seat first.
    WaitingStudent student = waitingList.dequeue();

    QDateTime start = QDateTime::currentDateTime();
    QDateTime expiry = start.addSecs(student.duration * 3600);

    booked[seat] = true;
    names[seat] = student.name;
    mobiles[seat] = student.mobile;
    userTypes[seat] = student.userType;
    durations[seat] = student.duration;
    bookingStart[seat] = start;
    bookingExpiry[seat] = expiry;

    totalBookings++;

    if (student.userType == "Student") {
        studentBookings++;
    } else {
        facultyBookings++;
    }

    recordHistory("WAITLIST-BOOKED", seat);

    saveBookings();
    saveStatistics();
    saveBookingHistory();

    updateSeats();
}

void MainWindow::updateWaitingList()
{
    if (waitingList.isEmpty()) {
        QMessageBox::information(
            this,
            "Waiting List",
            "The waiting list is currently empty."
            );
        return;
    }

    QString list;

    for (int i = 0; i < waitingList.size(); i++) {

        const WaitingStudent &student = waitingList.at(i);

        list += QString::number(i + 1) + ". ";
        list += student.name + " - ";
        list += student.mobile + " - ";
        list += student.userType + " - ";
        list += QString::number(student.duration) + " hours\n";
    }

    QMessageBox::information(
        this,
        "Waiting List",
        list
        );
}

void MainWindow::recordHistory(
    const QString &status,
    int seat
    )
{
    QString entry =
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        + "|" + status
        + "|" + QString::number(seat)
        + "|" + names[seat]
        + "|" + mobiles[seat]
        + "|" + userTypes[seat]
        + "|" + QString::number(durations[seat]);

    bookingHistory.append(entry);
}
void MainWindow::saveBookings()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/bookings.txt";

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(
            this, "File Error",
            "Could not save bookings.txt"
            );
        return;
    }

    QTextStream out(&file);

    for (int i = 1; i <= TOTAL_SEATS; i++) {
        if (booked[i]) {
            out << i << "|"
                << names[i] << "|"
                << mobiles[i] << "|"
                << userTypes[i] << "|"
                << durations[i] << "|"
                << bookingStart[i].toString(Qt::ISODate) << "|"
                << bookingExpiry[i].toString(Qt::ISODate)
                << "\n";
        }
    }
}

void MainWindow::loadBookings()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/bookings.txt";

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QStringList parts = in.readLine().split('|');

        if (parts.size() < 5) {
            continue;
        }

        bool ok = false;
        int seat = parts[0].toInt(&ok);

        if (!ok || seat < 1 || seat > TOTAL_SEATS) {
            continue;
        }

        booked[seat] = true;
        names[seat] = parts[1];
        mobiles[seat] = parts[2];
        userTypes[seat] = parts[3];
        durations[seat] = parts[4].toInt();

        if (parts.size() >= 7) {
            bookingStart[seat] =
                QDateTime::fromString(parts[5], Qt::ISODate);

            bookingExpiry[seat] =
                QDateTime::fromString(parts[6], Qt::ISODate);
        } else {
            bookingStart[seat] = QDateTime::currentDateTime();

            bookingExpiry[seat] =
                bookingStart[seat].addSecs(durations[seat] * 3600);
        }
    }

    file.close();
}

void MainWindow::saveStatistics()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/statistics.txt";

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);

    out << totalBookings << "\n";
    out << studentBookings << "\n";
    out << facultyBookings << "\n";
    out << totalCancellations << "\n";
}

void MainWindow::loadStatistics()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/statistics.txt";

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);

    if (!in.atEnd()) totalBookings = in.readLine().toInt();
    if (!in.atEnd()) studentBookings = in.readLine().toInt();
    if (!in.atEnd()) facultyBookings = in.readLine().toInt();
    if (!in.atEnd()) totalCancellations = in.readLine().toInt();
}

void MainWindow::saveBookingHistory()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/booking_history.txt";

    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(
            this, "History Error",
            "Could not save booking history."
            );
        return;
    }

    QTextStream out(&file);

    for (const QString &entry : bookingHistory) {
        out << entry << "\n";
    }
}

void MainWindow::loadBookingHistory()
{
    QString path =
        QCoreApplication::applicationDirPath() + "/booking_history.txt";

    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString entry = in.readLine();

        if (!entry.isEmpty()) {
            bookingHistory.append(entry);
        }
    }
}

void MainWindow::showBookingHistory()
{
    QDialog dialog(this);

    dialog.setWindowTitle("Booking History");
    dialog.resize(900, 450);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *heading = new QLabel("BOOKING HISTORY");
    heading->setAlignment(Qt::AlignCenter);

    heading->setStyleSheet(
        "font-size:20px;font-weight:bold;padding:8px;"
        );

    layout->addWidget(heading);

    QTableWidget *table = new QTableWidget(&dialog);

    table->setColumnCount(7);

    table->setHorizontalHeaderLabels({
        "Date & Time",
        "Status",
        "Seat",
        "Name",
        "Mobile",
        "User Type",
        "Duration"
    });

    table->setRowCount(bookingHistory.size());

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);

    for (int row = 0; row < bookingHistory.size(); row++) {
        QStringList fields =
            bookingHistory[row].split('|');

        for (int col = 0; col < 7; col++) {
            QString value;

            if (col < fields.size()) {
                value = fields[col];

                if (col == 6) {
                    value += " hours";
                }
            }

            table->setItem(
                row, col,
                new QTableWidgetItem(value)
                );
        }
    }

    table->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents
        );

    table->horizontalHeader()->setStretchLastSection(true);

    layout->addWidget(table);

    QPushButton *clearButton = new QPushButton("CLEAR HISTORY");
    QPushButton *closeButton = new QPushButton("Close");

    clearButton->setStyleSheet(
        "background: #dc3545; color: white;"
        "padding: 9px; font-weight: bold;"
        "border-radius: 5px;"
        );

    layout->addWidget(clearButton);
    layout->addWidget(closeButton);

    connect(
        clearButton, &QPushButton::clicked,
        &dialog,
        [this, table, &dialog]() {

            QMessageBox::StandardButton answer =
                QMessageBox::question(
                    &dialog,
                    "Clear History",
                    "Are you sure you want to clear all booking history?",
                    QMessageBox::Yes | QMessageBox::No
                    );

            if (answer == QMessageBox::Yes) {

                bookingHistory.clear();

                saveBookingHistory();

                table->setRowCount(0);

                QMessageBox::information(
                    &dialog,
                    "Success",
                    "Booking history cleared successfully!"
                    );
            }
        }
        );

    connect(
        closeButton, &QPushButton::clicked,
        &dialog, &QDialog::accept
        );

    dialog.exec();
}