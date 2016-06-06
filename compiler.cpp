#include "compiler.h"

#include <QStringList>
#include <QDebug>

#include <QTime>
#include <cmath>

Compiler::Compiler(Memory *mem, DebugPanel *panel, QProgressBar *mBar, CommandFactory *factory)
{
    this->mMemory = mem;
    this->mGenCode = new CodeGenerator();
    this->out = NULL;
    this->mDpanel = panel;
    this->mBar = mBar;
    this->mFactory = factory;
}

Compiler::~Compiler()
{
    delete mGenCode;
}

void Compiler::setOutPutConsole(QTextEdit *ptr)
{
    this->out = ptr;
}

QString Compiler::parseLabel(QString &strSource)
{
    strSource = strSource.replace("\n", " ");
    QStringList slCommandPair = strSource.split(" ");
    QMap<QString, int> mapLabel;

    for(int i = 0; i < slCommandPair.size(); i ++) {
        QString var = slCommandPair.at(i);
        if(var.indexOf(":") > 0) {
            QString lbl = var.remove(var.length() - 1, 1);
            int line = i / 2;
            slCommandPair.removeAt(i);
            mapLabel.insert(lbl, line);
        }
    }

    for(auto itr = mapLabel.begin(); itr != mapLabel.end(); itr ++) {
        for(int i = 0; i < slCommandPair.size(); i ++) {
            if(slCommandPair.at(i) == itr.key()) {
                slCommandPair.replace(i, QString::number(itr.value()));
            }
        }
    }

    strSource = slCommandPair.join(" ");

    return strSource;
}

bool Compiler::checkCommand(QString &strSource)
{
    QStringList slCommandPair = strSource.split(" ");
    int indexCmd = 0;
    indexCheckCommand = 0;
    QString strCmd = NULL, strArg = NULL , strCode = NULL;
    bool mainCmd = false;
    while (slCommandPair.size() > indexCmd) {
        mainCmd = false;
        strCmd = slCommandPair.at( indexCmd );
        if(strCmd == "nop" || strCmd == "ret" || strCmd == "htl" || strCmd == "out" || strCmd == "in"){

        }else if(strCmd == "jrnz" || strCmd == "mov"){
            //не сделанно
            return false;
        }else{
            strArg = slCommandPair.at( ++indexCmd );
            if( strArg.contains('r') || strArg.contains('R')) //проверка операнда на регистр или число
                strCode = mGenCode->getCode(strCmd + "R");
            else
                strCode = mGenCode->getCode(strCmd);
            AbstactCommand *cmdImpl = mFactory->createCommand( strCode.toInt() );
            mainCmd = cmdImpl->check( strArg );
            if(!mainCmd)
                return false;
            ++indexCmd;
            ++indexCheckCommand;
            continue;
        }
        strCmd = slCommandPair.at( ++indexCmd);
        strArg = slCommandPair.at( ++indexCmd);
        if( strArg.contains('r') || strArg.contains('R')) //проверка операнда на регистр или число
            strCode = mGenCode->getCode(strCmd + "R");
        else
            strCode = mGenCode->getCode(strCmd);
        AbstactCommand *cmdImpl = mFactory->createCommand( strCode.toInt() );
        mainCmd = cmdImpl->check( strArg );
        if(!mainCmd)
            return false;
        ++indexCmd;
        indexCheckCommand += 2;
    }
    return true;
}

void Compiler::exec(QString &strSource)
{
    mBar->setStyleSheet("QProgressBar::chunk {background-color: rgb(0, 255, 0);}");
    QStringList debugList;
    QTime startTime = QTime::currentTime();
    strSource = parseLabel(strSource);
    if( !checkCommand(strSource) ){
        qDebug() << "Checked false";
        out->append("ТЕСТРИТОВАНИЯ. Ошибка компиляции в строке " + QString::number( indexCheckCommand + 1 ) );
        mBar->setStyleSheet("QProgressBar::chunk {background-color: rgb(255, 0, 0);}");
        mBar->setValue(mBar->maximum());
        return;
    }
    QStringList slCommandPair = strSource.split(" ");
    qDebug() << "After parse:" << slCommandPair;
    mBar->setMaximum(slCommandPair.size());
    mBar->setValue(0);
    QString isRegistr, strCode, strCmd, strArg = "000", typeAdrr = "0", debugArg = " ", debugCmd = " ";
    int indexCmd = 0;
    for(int i = 0; i < slCommandPair.size(); i ++, mBar->setValue(i)) {
        strCmd = slCommandPair.at(i);
        debugCmd = slCommandPair.at(i);
        strCmd = strCmd.toLower();
        if(strCmd == "nop"){
            mMemory->set(indexCmd, "000000");
            indexCmd ++;
            debugList.append(debugCmd + " " + debugArg);
            continue;
        }else if(strCmd == "in"){
            mMemory->set(indexCmd, "010000");
            indexCmd ++;
            debugList.append(debugCmd + " " + debugArg);
            continue;
        }else if(strCmd == "out"){
            mMemory->set(indexCmd, "020000");
            indexCmd ++;
            debugList.append(debugCmd + " " + debugArg);
            continue;
        }else if(strCmd == "htl"){
            mMemory->set(indexCmd, "090000");
            indexCmd ++;
            debugList.append(debugCmd + " " + debugArg);
            continue;
        }
        else if(strCmd == "ret"){
            mMemory->set(indexCmd, "080000");
            indexCmd ++;
            debugList.append(debugCmd);
            continue;
        }
        else if(strCmd == "jrnz"){
            //команда выглядит как JRNZ R,M
            //где R номер регистра, M - метка
            //в память пишется в одну ячейку
            // ccRmmm
            // где cc код команды, R номер регистр, mmm номер PC
            // в коде я сам по вызову команды для JRNZ
            // я сам с ячейки памяти возьму Rmmm, в параметры можно при вызове функции не передавать
            // но надо распарсить и записать это в память
            qDebug() << "JRNZ еще не готов. return из компилятора";
            return;
        }
        isRegistr = slCommandPair.at(i + 1);
        if( isRegistr.contains('r') || isRegistr.contains('R')) //проверка операнда на регистр или число
            strCode = mGenCode->getCode(strCmd + "R");
        else
            strCode = mGenCode->getCode(strCmd);
        //qDebug() << "CODE:" << strCode;
        if(strCode != "-1") {
            if((i + 1) < slCommandPair.size() ) {
                if(mGenCode->getCode(slCommandPair.at(i + 1)) == "-1") {
                    i ++;
                    strArg = slCommandPair.at(i);
                    debugArg = strArg;
                    typeAdrr = getAddresType((QString)strArg[0] + (QString)strArg[1]);//переделал для Косвенно-регистровой
                    getAddresCode(strArg, typeAdrr);
                }
            }

            if(strArg.size() < 2) { //два if что бы если размер 1 то + 00
                strArg.push_front("0");
            }
            if(strArg.size() < 3) { // если размер 2 то только + 0
                strArg.push_front("0");
            }
            strArg.replace('r', '0');
            QString cmd = strCode + typeAdrr + strArg;
            debugList.append(debugCmd + " " + debugArg);
            mMemory->set(indexCmd, cmd);
            indexCmd ++;
        } else {
            if(out) {
                QString outPutMsg = "Ошибка компиляции в строке: " + QString::number(indexCmd + 1);
                mBar->setStyleSheet("QProgressBar::chunk {background-color: rgb(255, 0, 0);}");
                mBar->setValue(mBar->maximum());
                out->append(outPutMsg);
                return;
            }
        }
    }
    QTime endTime = QTime::currentTime();
    if(out) {
        out->clear();
        QString secnd = QString::number(abs(endTime.second() - startTime.second()));
        QString min = QString::number(abs(endTime.minute() - startTime.minute()));
        QString msecnd = QString::number(abs(endTime.msec() - startTime.msec()));
        QString outPutMsg = "Компиляция прошла успешно время: " +
                formatTime(min, 2) + ":" + formatTime(secnd, 2) + ':' + formatTime(msecnd, 3) ;
        out->append(outPutMsg);
    }
    mDpanel->updateCode(debugList);
}

QString Compiler::getAddresType(QString str)
{
    if(str[0] == '#') {
        return "1";
    } else if(str[0] == '@' && !str.contains('r') && !str.contains('R')) {
        return "2";
    } else if(str[0] == '['){
        return "3";
    } else if(str[0] == '@' && ( str.contains('r') || str.contains('R'))){
        return "4";
    } else if(str[0] == '-' && str.contains('@')){
        return "6";
    }
    //адресация "5" проверяется после вызова этой функции в exec()
    return "0";
}

void Compiler::getAddresCode(QString &strArg, QString &typeAdrr)
{
    if(typeAdrr == "3"){ //в относительной адресации у нас число стоит между скобками
        strArg = strArg.remove(0,1); //по этому надо удалить их но оставить число [xxx]
        for(int n = 0; n < strArg.size(); n++){
            if(strArg[n] == ']'){
                strArg.remove(n, 1);
                break;
            }
        }
    }else if(typeAdrr == "4" && strArg[3] == '+'){ //проверка на алресацию Индексная с постинкрементом
        typeAdrr = "5";
        strArg.remove(3, 1);
        strArg.remove(0, 1);
    }else if(typeAdrr == "6"){
        strArg.remove(1, 1);
        strArg.remove(0, 1);
    }else if(typeAdrr == "1" || typeAdrr == "2" || typeAdrr == "4"){
        strArg = strArg.remove(0, 1);
    }
}

QString Compiler::formatTime(QString str, int cnt)
{
    while(str.size() < cnt) {
        str.push_front("0");
    }
    return str;
}
