// Copyright (C) 2017, GReaperEx(Marios F.)
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CExtended3State.h"


#include <string>
#include <iomanip>

using namespace std;

CExtended3State::CExtended3State(int size)
: IBasicState(size, 10000, false, true), curPtrPos(0), IP(1), storagePos(0)
{}

CExtended3State::~CExtended3State()
{}

void CExtended3State::translate(std::istream& input)
{
    string tokens = "<>+-.,[]@$!{}~^&|?()*/=_%XxMmLl:#0123456789ABCDEFabcdef";
    char c;
    bool endFound = false;

    ++curPtrPos;
    while (input >> c) {
        if (tokens.find(c) != string::npos) {
            CellType temp = { 0 };
            temp.c8 = c;
            setCell(curPtrPos++, temp);

            if (c == '@') {
                endFound = true;
                break;
            }
        }
    }

    //! Adds the '@' instruction if none was given
    if (!endFound) {
        CellType temp = { 0 };
        temp.c8 = '@';
        setCell(curPtrPos++, temp);
    }

    initPtrPos = curPtrPos;
    //! Uses any data left to initialize the tape
    //! Understands escape sequences, symbol, octal and hex
    //! Only hex values can be > 255( technically, octal too but not for much )
    while (input.get(c)) {
        CellType temp = { 0 };
        if (c == '\\') {
            if (!input.get(c)) {
                throw runtime_error("Data: Expected symbol after \'\\\'.");
            }
            switch (c)
            {
            case 'a':
                temp.c8 = '\a';
            break;
            case 'b':
                temp.c8 = '\b';
            break;
            case 'f':
                temp.c8 = '\f';
            break;
            case 'n':
                temp.c8 = '\n';
            break;
            case 'r':
                temp.c8 = '\r';
            break;
            case 't':
                temp.c8 = '\t';
            break;
            case 'v':
                temp.c8 = '\v';
            break;
            case '\\':
                temp.c8 = '\\';
            break;
            case '\'':
                temp.c8 = '\'';
            break;
            case '\"':
                temp.c8 = '\"';
            break;
            case '?':
                temp.c8 = '?';
            break;
            case 'x':
                for (int i = 0; i < getCellSize()*2; i++) {
                    int c = tolower(input.peek());
                    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                        break;
                    }
                    c = input.get();

                    temp.c64 <<= 4;
                    if (c >= 'a' && c <= 'f') {
                        temp.c64 += (c - 'a') + 10;
                    } else {
                        temp.c64 += (c - '0');
                    }
                }
            break;
            default:
                if (!(c >= '0' && c <= '7')) {
                    throw runtime_error("Data: Unexpected symbol after \'\\\'.");
                }
                for (int i = 0; i < 3; i++) {
                    int c = tolower(input.peek());
                    if (!(c >= '0' && c <= '7')) {
                        break;
                    }
                    c = input.get();

                    temp.c64 <<= 3;
                    temp.c64 += (c - '0');
                }
            }
        } else {
            temp.c8 = c;
        }

        setCell(curPtrPos++, temp);
    }
    curPtrPos = initPtrPos;
}

void CExtended3State::run()
{
    CellType curCell = { 0 };

    while (curCell.c8 != '@') {
        curCell = getCell(IP++);
        switch (curCell.c8)
        {
        case '<':
            --curPtrPos;
        break;
        case '>':
            ++curPtrPos;
        break;
        case '+':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                ++temp.c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '-':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                --temp.c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '.':
        {
            CellType temp = getCell(curPtrPos);
            cout.put(temp.c8);
        }
        break;
        case ',':
        {
            CellType temp = { 0 };
            temp.c8 = cin.get();
            if (!isLocked(curPtrPos)) {
                setCell(curPtrPos, temp);
            }
        }
        break;
        case '[':
        {
            CellType selCell = getCell(curPtrPos);
            if (selCell.c64 == 0) {
                int level = 1;
                int newIP = IP;
                while (level != 0) {
                    CellType temp = getCell(newIP++);
                    if (temp.c8 == ']') {
                        --level;
                    } else if (temp.c8 == '@') {
                        break;
                    }
                }
                if (level == 0) {
                    IP = newIP;
                } else {
                    CellType temp = getCell(IP-1);
                    temp.c8 = 1;
                    setCell(IP-1, temp);
                }
            }
        }
        break;
        case ']':
        {
            CellType selCell = getCell(curPtrPos);
            if (selCell.c64 != 0) {
                int level = 1;
                int newIP = IP-2;
                while (level != 0 && newIP >= 0) {
                    CellType temp = getCell(newIP--);
                    if (temp.c8 == '[') {
                        --level;
                    }
                }
                if (level == 0) {
                    IP = newIP+2;
                } else {
                    CellType temp = getCell(IP-1);
                    temp.c8 = 1;
                    setCell(IP-1, temp);
                }
            }
        }
        break;
        case '@':
        break;
        case '$':
            if (!isLocked(storagePos)) {
                setCell(storagePos, getCell(curPtrPos));
            }
        break;
        case '!':
            if (!isLocked(curPtrPos)) {
                setCell(curPtrPos, getCell(storagePos));
            }
        break;
        case '{':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 <<= 1;
                setCell(curPtrPos, temp);
            }
        break;
        case '}':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 >>= 1;
                setCell(curPtrPos, temp);
            }
        break;
        case '~':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 = ~temp.c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '^':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 ^= getCell(0).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '&':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 &= getCell(0).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '|':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 |= getCell(0).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '?':
            IP = curPtrPos;
        break;
        case '(':
            for (int i = curPtrPos; i < getCellCount()-1; i++) {
                if (!isLocked(i)) {
                    setCell(i, getCell(i+1));
                }
            }
        break;
        case ')':
        {
            //! Avoids lengthening the tape if it doesn't have to
            CellType temp = getCell(getCellCount()-1);
            int i = getCellCount();
            if (temp.c64 == 0) {
                --i;
            }

            for (; i > curPtrPos; i--) {
                if (!isLocked(i)) {
                    setCell(i, getCell(i-1));
                }
            }
            temp.c64 = 0;
            if (!isLocked(i)) {
                setCell(i, temp);
            }
        }
        break;
        case '*':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 *= getCell(storagePos).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '/':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 /= getCell(storagePos).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '=':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 += getCell(storagePos).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '_':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 -= getCell(storagePos).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case '%':
            if (!isLocked(curPtrPos)) {
                CellType temp = getCell(curPtrPos);
                temp.c64 %= getCell(storagePos).c64;
                setCell(curPtrPos, temp);
            }
        break;
        case 'X':
            prevPtrs.push_back(curPtrPos);
            curPtrPos = IP-1;
        break;
        case 'x':
            if (prevPtrs.empty()) {
                curPtrPos = initPtrPos;
            } else {
                curPtrPos = prevPtrs.back();
                prevPtrs.pop_back();
            }
        break;
        case 'M':
            storagePos = curPtrPos;
        break;
        case 'm':
            storagePos = 0;
        break;
        case 'L':
            lockMap[curPtrPos] = true;
        break;
        case 'l':
            lockMap[curPtrPos] = false;
        break;
        case ':':
            curPtrPos += (int)getCell(curPtrPos).c32; //! Using c64 is meaningless, curPtrPos is 32-bit
        break;
        case '#':
            ++curPtrPos;
            for (;;) {
                CellType temp = getCell(curPtrPos++);
                if (temp.c8 == '#') {
                    break;
                } else if (temp.c8 == '@') {
                    --curPtrPos;
                    break;
                }
            }
        break;
        default:
            curCell.c8 = toupper(curCell.c8);
            if (curCell.c8 >= '0' && curCell.c8 <= '9') {
                CellType temp = getCell(curPtrPos);

                temp.c8 = (curCell.c8 - '0') << 4;
                if (!isLocked(curPtrPos)) {
                    setCell(curPtrPos, temp);
                }
            } else if (curCell.c8 >= 'A' && curCell.c8 <= 'F') {
                CellType temp = getCell(curPtrPos);

                temp.c8 = (curCell.c8 - 'A' + 10) << 4;
                if (!isLocked(curPtrPos)) {
                    setCell(curPtrPos, temp);
                }
            }
        }
    }
}

void CExtended3State::compile(std::ostream&)
{
    throw runtime_error("Extended Brainfuck Type II can't be compiled.");
}

