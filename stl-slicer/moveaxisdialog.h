#ifndef MOVEAXISDIALOG_H
#define MOVEAXISDIALOG_H

#include <QDialog>
#include <QObject>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

class MoveAxisDialog : public QDialog
{
    Q_OBJECT

public:
    MoveAxisDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        // Create the dialog widgets
        QLabel *axisLabel = new QLabel("Axis:");
        QComboBox *axisComboBox = new QComboBox;
        axisComboBox->addItem("X");
        axisComboBox->addItem("A");
        axisComboBox->addItem("Y");
        axisComboBox->addItem("Z");
        axisComboBox->setObjectName("axisComboBox");
        QLabel *positionLabel = new QLabel("Position:");
        QLineEdit *positionLineEdit = new QLineEdit;
        positionLineEdit->setText("0");
        positionLineEdit->setObjectName("positionLineEdit");
        QLabel *feedrateLabel = new QLabel("Feedrate:(mm/min)");
        QLineEdit *feedrateLineEdit = new QLineEdit;
        feedrateLineEdit->setText("600");
        // Create the dialog buttons
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        // Connect the buttons to the dialog slots
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // Create the dialog layout
        QVBoxLayout *mainLayout = new QVBoxLayout;
        QHBoxLayout *axisLayout = new QHBoxLayout;
        QHBoxLayout *positionLayout = new QHBoxLayout;

        axisLayout->addWidget(axisLabel);
        axisLayout->addWidget(axisComboBox);

        positionLayout->addWidget(positionLabel);
        positionLayout->addWidget(positionLineEdit);

        mainLayout->addLayout(axisLayout);
        mainLayout->addLayout(positionLayout);
        mainLayout->addWidget(buttonBox);

        setLayout(mainLayout);
        setWindowTitle("Custom Dialog");
    }

    QString getAxis() const
    {
        QComboBox *axisComboBox = findChild<QComboBox *>("axisComboBox");
        return axisComboBox->currentText();
    }

    float getPosition() const
    {
        QLineEdit *positionLineEdit = findChild<QLineEdit *>("positionLineEdit");
        return positionLineEdit->text().toFloat();
    }
};

#endif // MOVEAXISDIALOG_H
