#ifndef NFRUSPMAINWINDOW_H
#define NFRUSPMAINWINDOW_H

#include <QWidget>
#include <QDir>
#include <QPushButton>
#include <QComboBox>
#include <QStringList>
#include <QTableWidget>
#include <QProcess>
#include <QLineEdit>
#include <QCloseEvent>

#include <deque>
#include <vector>


// TODO: 
//  * queue display/append/saving/editing
//  * backend selection (mplayer, xine, ...) via command line and player error handling







class NFrusPMainWindow : public QWidget
{
    Q_OBJECT

    enum PlayModeType
    {
      stopAfterEach = 0,
      continueSequentially = 1,
      continueRandomly = 2,
      repeatSong = 3      
    };
    
public:
    NFrusPMainWindow();
    ~NFrusPMainWindow(){}
    
protected:
  
    void closeEvent(QCloseEvent * e);

private slots:
    void addDirButtonSlot();
    void clearButtonSlot();
    void pausePlayButtonSlot();
    void playModeSlot(int index);
    void searchSlot();

    void selectFile(int row, int column);

    void playerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void createFilesTable();
    void appendTableRow(QFileInfo const & info);
    void appendTableRow(QString fileName, QString size, QString fullPath);
    void removeDuplicates();
    void readList();
    void writeList();
    void play();
    void stop();

    // gui elements
    QPushButton  * addDirButton;
    QPushButton  * clearButton;

    QPushButton  * pausePlayButton;
    QComboBox    * playModeSelection;

    QLineEdit    * searchTextEdit;
    int lastHitPosition; // -1 is invalid (no hit or initial value)
    QString currentSearchString;

    // object state
    QTableWidget * filesTable;  // table of music files (with hidden fields)
    bool filesTableChanged;
    std::deque<int> playQueue; // indexes into the filesTable that had been played or are about to be played
    int currentPlayQueueIndex; // index into the playQueue of the currently playing file (-1 in invalid case)
    QProcess player;
    PlayModeType playMode;
    bool inStopFunction;
};



#endif
