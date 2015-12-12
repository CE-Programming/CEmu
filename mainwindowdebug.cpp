#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <iostream>
#include <QDockWidget>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStyledItemDelegate>

#include "aboutwindow.h"
#include "settings.h"
#include "emuthread.h"
#include "core/debug.h"

// custom item delegate class
class NoFocusDelegate : public QStyledItemDelegate
{
protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

void NoFocusDelegate::paint(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption(option);
    if (itemOption.state & QStyle::State_HasFocus)
        itemOption.state = itemOption.state ^ QStyle::State_HasFocus;
    QStyledItemDelegate::paint(painter, itemOption, index);
}

void MainWindow::initDebugger() {
    // Setup the model for the debugger
    QStandardItemModel *model = new QStandardItemModel(0, 3);
    for (int column = 0; column < 3; ++column) {
        QStandardItem *item = new QStandardItem( QString("") );
        item->setEditable( false );
        model->setItem(0, column, item);
    }

    ui->disasmView->setModel( model );

    ui->disasmView->setSelectionBehavior( QAbstractItemView::SelectRows );
    ui->disasmView->setShowGrid( false );
    ui->disasmView->setItemDelegate( new NoFocusDelegate() );
    ui->disasmView->horizontalHeader()->hide();
    ui->disasmView->horizontalHeader()->setStretchLastSection( true );
    ui->disasmView->verticalHeader()->sectionResizeMode( QHeaderView::Fixed );
    ui->disasmView->verticalHeader()->setDefaultSectionSize( 15 );
    ui->disasmView->verticalHeader()->hide();

    enableDebugger( false );
}

void MainWindow::enableDebugger( bool enable ) {
    ui->scrollArea1->setEnabled( enable );
    ui->scrollArea2->setEnabled( enable );
    ui->tabDebugging->setEnabled( enable );
    ui->buttonBreakpoint->setEnabled( enable );
    ui->buttonGoto->setEnabled( enable );
    ui->buttonStep->setEnabled( enable );
    ui->buttonStepOver->setEnabled( enable );
    ui->registerGroup->setEnabled( enable );
    ui->flagGroup->setEnabled( enable );
    ui->cpuGroup->setEnabled( enable );
    ui->intrptGroup->setEnabled( enable );
    ui->displayGroup->setEnabled( enable );
}

void MainWindow::checkDebuggerState() {
    QPixmap pix;
    QIcon icon;

    if( gui_debug.stopped == false ) {
        ui->buttonRun->setText( "Stop" );
        pix.load( ":/icons/resources/icons/stop.png" );
    } else {
        ui->buttonRun->setText( "Run" );
        pix.load( ":/icons/resources/icons/run.png" );
    }

    emu_debug.stopped = gui_debug.stopped;

    icon.addPixmap(pix);
    ui->buttonRun->setIcon(icon);
    ui->buttonRun->setIconSize(pix.size());

    enableDebugger( gui_debug.stopped );

    gui_debug.stopped = !gui_debug.stopped;

    emu_thread->enterDebugger();
    raiseDebugger();
}

void MainWindow::raiseDebugger()
{
    // The GUI is in stopped state
    gui_debug.stopped = true;

    // populate the information on the degbug window
    this->populateDebugWindow();
}

static QString int2hex(uint32_t a, uint8_t l) {
  return QString("%1").arg(a, l, 16, QLatin1Char( '0' )).toUpper();
}

void MainWindow::populateDebugWindow()
{
  eZ80registers_t *CEreg = &emu.asic_ptr->cpu->registers;
  asic_state_t *CEasic = emu.asic_ptr;
  eZ80cpu_t *CEcpu = emu.asic_ptr->cpu;

  ui->afregView->setText( int2hex(CEreg->AF, 4) );
  ui->hlregView->setText( int2hex(CEreg->HL, 6) );
  ui->deregView->setText( int2hex(CEreg->DE, 6) );
  ui->bcregView->setText( int2hex(CEreg->BC, 6) );
  ui->ixregView->setText( int2hex(CEreg->IX, 6) );
  ui->iyregView->setText( int2hex(CEreg->IY, 6) );

  ui->af_regView->setText( int2hex(CEreg->_AF, 4) );
  ui->hl_regView->setText( int2hex(CEreg->_HL, 6) );
  ui->de_regView->setText( int2hex(CEreg->_DE, 6) );
  ui->bc_regView->setText( int2hex(CEreg->_BC, 6) );

  ui->mbregView->setText( int2hex(CEreg->MBASE, 2) );
  ui->pcregView->setText( int2hex(CEreg->PC, 6) );
  ui->spsregView->setText(  int2hex(CEreg->SPS, 4) );
  ui->splregView->setText(  int2hex(CEreg->SPL, 6) );

  ui->checkZ->setChecked( CEreg->flags.Z );
  ui->checkC->setChecked( CEreg->flags.C );
  ui->checkS->setChecked( CEreg->flags.S );
  ui->checkPV->setChecked( CEreg->flags.PV );
  ui->checkHC->setChecked( CEreg->flags.H );
  ui->check3->setChecked( CEreg->flags._3 );
  ui->checkZ->setChecked( CEreg->flags._5 );

  ui->checkIEF1->setChecked( CEcpu->IEF1 );
  ui->checkIEF2->setChecked( CEcpu->IEF2 );

  ui->iregView->setText( int2hex(CEreg->I, 4) );
  ui->rregView->setText( int2hex(CEreg->R, 2) );
}
