

#include <NFrusPMainWindow.h>

#include <QGridLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QDirIterator>
#include <QShortcut>

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>





#ifdef QT_DEBUG
#define DEBUG
#endif





NFrusPMainWindow::NFrusPMainWindow()
    :QWidget(0)
    ,lastHitPosition(-1)
    ,filesTableChanged(false)
    ,currentPlayQueueIndex(-1)
    ,player(this)
    ,inStopFunction(false)
{
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    // buttons
    addDirButton = new QPushButton("&Add dir");
    connect(addDirButton, SIGNAL(clicked()), this, SLOT(addDirButtonSlot()));
    pausePlayButton = new QPushButton("&Pause");
    connect(pausePlayButton, SIGNAL(clicked()), this, SLOT(pausePlayButtonSlot()));
    clearButton = new QPushButton("&Clear");
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearButtonSlot()));
    playModeSelection = new QComboBox();
    playModeSelection->addItem("Stop after each");
    playModeSelection->addItem("Continue sequentially");
    playModeSelection->addItem("Continue randomly");
    playModeSelection->addItem("Repeat Song");
    playModeSelection->setCurrentIndex(continueRandomly);
    playMode = continueRandomly;
    connect(playModeSelection, SIGNAL(activated(int)), this, SLOT(playModeSlot(int)));
    mainLayout->addWidget(addDirButton,           0, 0);
    mainLayout->addWidget(clearButton,            0, 1);
    mainLayout->addWidget(pausePlayButton,        0, 2);
    mainLayout->addWidget(playModeSelection,      0, 3);

    // search line
    searchTextEdit = new QLineEdit();
    connect(searchTextEdit, SIGNAL(returnPressed()), this, SLOT(searchSlot()));
    mainLayout->addWidget(searchTextEdit, 1, 0, 1, 4);
    QShortcut * shortcut = new QShortcut(QKeySequence(tr("Ctrl+S", "Place cursor in search field")), this);
    connect(shortcut, SIGNAL(activated()), searchTextEdit, SLOT(setFocus()));

    // files table
    filesTable = new QTableWidget(0, 3);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    filesTable->setColumnHidden(2, true);
    QStringList labels;
    labels << tr("Filename") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->setShowGrid(false);
    connect(filesTable, SIGNAL(cellActivated(int,int)), this, SLOT(selectFile(int,int)));
    mainLayout->addWidget(filesTable, 2, 0, 1, 4);

    
    setLayout(mainLayout);
    setWindowTitle("NFrusP");
    resize(800, 600);
    
    connect( & player, 
             SIGNAL(finished(int, QProcess::ExitStatus)), 
             this, 
             SLOT(playerProcessFinished(int, QProcess::ExitStatus))
           );
    
    readList();
}









void NFrusPMainWindow::closeEvent(QCloseEvent * event)
{
  stop(); 
  if(filesTableChanged) writeList();
  event->accept();
}










void NFrusPMainWindow::searchSlot()
{
  if(searchTextEdit->text() not_eq currentSearchString)
  { // reinit search
    lastHitPosition = -1;
    currentSearchString = searchTextEdit->text();
  }
  // search must wrap around, but one search may go for at max a table length before finding nothing
  int searchIndex = std::max(lastHitPosition + 1, 0);
  for(int count = 0; count < filesTable->rowCount(); ++count, ++searchIndex)
  {
    if( filesTable->item
         ((searchIndex % filesTable->rowCount()), 0)->text().contains
            (currentSearchString, Qt::CaseInsensitive)
      )
    {
      filesTable->selectRow(searchIndex % filesTable->rowCount());
      lastHitPosition = searchIndex % filesTable->rowCount(); 
      break;
    }
  }
}












void NFrusPMainWindow::pausePlayButtonSlot()
{
  if( (player.state() not_eq QProcess::NotRunning) and 
      (pausePlayButton->text() == "&Pause")
    )
  {
    stop();    
    pausePlayButton->setText("&Play");
  }
  else if (pausePlayButton->text() == "&Play")
  {
    play();
    pausePlayButton->setText("&Pause");
  }
}








void NFrusPMainWindow::play()
{
  if( (not playQueue.empty()) and (currentPlayQueueIndex < int(playQueue.size())) )
  {
    QString fileToPlay = filesTable->item(playQueue[currentPlayQueueIndex], 2)->text();
    #ifdef DEBUG
    std::cerr << "[NFrusp] start playing: " << fileToPlay.toStdString() << " using /usr/bin/mplayer." << std::endl;
    #endif
    QString program = "/usr/bin/mplayer";
    QStringList arguments;
    arguments << "-quiet" << fileToPlay;
    player.start(program, arguments);
  }
}









void NFrusPMainWindow::stop()
{
  inStopFunction = true;
  if(player.state() not_eq QProcess::NotRunning)
  {
    // write quit to mplayer pipe and wait for it to finish
    player.write("q");
    if(not player.waitForFinished(1000)) player.kill(); // wait for a second at max
  }
  inStopFunction = false;
}










void NFrusPMainWindow::playerProcessFinished(int exitCode, QProcess::ExitStatus)
{
  #ifdef DEBUG
  std::cerr << "[NFrusp] player stopped with exit code: " << exitCode << std::endl;
  #endif

  // first handle normal exit (for example because song stopped or because stop()-Funktion was issued)
  if(exitCode == 0)
  {
    if(not inStopFunction )
    {
      int songCount = filesTable->rowCount();
      if(songCount > 0)
      {
        if(playMode == continueSequentially)
        {
          if(not playQueue.empty()) playQueue.push_back( (playQueue[currentPlayQueueIndex] + 1) % songCount );
          else                      playQueue.push_back( 0 );
          currentPlayQueueIndex = int(playQueue.size()) - 1;
          filesTable->selectRow(playQueue[currentPlayQueueIndex]);
          play();
        }
        else if(playMode == continueRandomly)
        {
	  // now we are searching for a random index (limited attempts), which is has not been part of playQueue
	  // or, if playQueue size is a multiple of the number of all songs, which has not been part more than
	  // this multiplicity
          int randomSong;	  
          for(std::size_t i = 0; i < 1000; ++i)
          {
            randomSong = rand() % songCount;
            
	    if( std::count(playQueue.begin(), playQueue.end(), randomSong) <=
	        (int(playQueue.size()) / songCount)
	      ) break;
          }
          
          playQueue.push_back(randomSong);
          currentPlayQueueIndex = int(playQueue.size()) - 1;
          filesTable->selectRow(playQueue[currentPlayQueueIndex]);
          play();
        }
        else if(playMode == repeatSong)
        {
          currentPlayQueueIndex = int(playQueue.size()) - 1;
          filesTable->selectRow(playQueue[currentPlayQueueIndex]);
          play();
        }
        // else "stop after each" does nothing
      }
    }
  }
  // TODO: handle error exit code! ******************************************************
  else
  {
    std::cerr << "[NFrusp] error exit from mplayer (not handled), "
                 "setup maybe wrong! Try playing the file manually with \"mplayer\"." << std::endl;
  }
}











void NFrusPMainWindow::playModeSlot(int index)
{
  playMode = PlayModeType(index);
}













void NFrusPMainWindow::appendTableRow(QString fileName, QString size, QString fullPath)
{
  QTableWidgetItem *fileNameItem = new QTableWidgetItem(fileName);
  fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
  QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
  sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
  sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);
  QTableWidgetItem * fullPathFileNameItem = new QTableWidgetItem(fullPath);

  int row = filesTable->rowCount();
  filesTable->insertRow(row);
  filesTable->setItem(row, 0, fileNameItem);
  filesTable->setItem(row, 1, sizeItem);
  filesTable->setItem(row, 2, fullPathFileNameItem);
}













void NFrusPMainWindow::appendTableRow(QFileInfo const & info)
{
  appendTableRow(info.completeBaseName(), tr("%1 KB").arg(int((info.size() + 1023) / 1024)), info.absoluteFilePath());
}









void NFrusPMainWindow::addDirButtonSlot()
{
  // this functions appends files, so we do not clear the table
  
  QString currentDirName = QFileDialog::getExistingDirectory(this, "Simple Player", QDir::currentPath());

  if(not currentDirName.isEmpty()) 
  {
    QDirIterator it(currentDirName, QDirIterator::Subdirectories);
    for(;it.hasNext(); it.next())
    {
      QFileInfo currentFileInfo = it.fileInfo();
      if (not currentFileInfo.isDir()) 
      {
        if( currentFileInfo.fileName().endsWith(".mp3") or 
            currentFileInfo.fileName().endsWith(".ogg") or 
            currentFileInfo.fileName().endsWith(".wav")
          )
        {
          appendTableRow(currentFileInfo);
        }
      }
    }
    stop();
    playQueue.clear();
    currentPlayQueueIndex = -1;
    filesTable->sortItems(0);  // sorts table by the 0th column
    removeDuplicates();
  }
  
  filesTableChanged = true;
}












void NFrusPMainWindow::clearButtonSlot()
{
  stop();
  filesTable->setRowCount(0);
  playQueue.clear();
  currentPlayQueueIndex = -1;
  filesTableChanged = true;
}












void NFrusPMainWindow::selectFile(int row, int /*column*/)
{
  stop();
  playQueue.push_back(row);
  currentPlayQueueIndex = int(playQueue.size()) - 1;
  play();
  pausePlayButton->setText("&Pause");  
}










void NFrusPMainWindow::removeDuplicates()
{
  #ifdef DEBUG
  std::cerr << "[NFrusp] removing dupes is not implemented!" << std::endl;
  #endif
}











const std::string saveLoadFileName("/.nfrusp.list.txt");





void NFrusPMainWindow::writeList()
{
  std::string homePath(getenv("HOME"));
  #ifdef DEBUG
  std::cerr << "[NFrusp] writing list to " << (homePath + saveLoadFileName) << std::endl;
  #endif
  std::ofstream outFile((homePath + saveLoadFileName).c_str());

  for(int i = 0; i < filesTable->rowCount(); ++i)
  {
    outFile << filesTable->item(i, 0)->text().toStdString() << '\n' 
            << filesTable->item(i, 1)->text().toStdString() << '\n' 
            << filesTable->item(i, 2)->text().toStdString() << '\n';
  }
}







void NFrusPMainWindow::readList()
{
  std::string homePath(getenv("HOME"));
  #ifdef DEBUG
  std::cerr << "[NFrusp] loading list from " << (homePath + saveLoadFileName) << std::endl;
  #endif
  std::ifstream inFile((homePath + saveLoadFileName).c_str());
  
  std::string fileName, size, fullPath;
  std::getline(inFile, fileName);
  std::getline(inFile, size);
  std::getline(inFile, fullPath);

  while(inFile)
  {
    appendTableRow(fileName.c_str(), size.c_str(), fullPath.c_str());
    std::getline(inFile, fileName);
    std::getline(inFile, size);
    std::getline(inFile, fullPath);
  }
}





