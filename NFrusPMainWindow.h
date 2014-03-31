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
    { // take care to number these beginning from zero with no "holes" as
      // we get only the index from the combo box they relate to
      stopAfterEach = 0,
      continueSequentially = 1,
      continueRandomly = 2,
      repeatSong = 3      
    };
    
    enum StateType
    {
      statePlaying,
      stateStopping,
      stateStopped
    };
    
    enum CustomEventType
    {
      playNextSong = QEvent::User + 0
    };
    
public:
    NFrusPMainWindow();
    ~NFrusPMainWindow(){}
    
protected:
  
    void closeEvent(QCloseEvent * e);
    void customEvent(QEvent * e); // to avoid race conditions

private slots:
    void addDirButtonSlot();
    void clearButtonSlot();
    void nextButtonSlot();
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
    void generateNextSongIndex();

    // gui elements
    QPushButton  * addDirButton;
    QPushButton  * clearButton;

    QPushButton  * pausePlayButton;
    QPushButton  * nextButton;
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
    StateType state;

    //QTableWidget * queueTable;  // TODO: queue user interface
};



#endif
