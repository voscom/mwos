#ifndef MWSTRINGLIST_H
#define MWSTRINGLIST_H

/**
 * Работа со списком в текстовой строке, где каждая линия списка разделена через delimiter
 */
class MWStringList {
public:
    String text="";
    char delimiter = ';';

    MWStringList() {}

    MWStringList(String listText,char listDelimiter) {
        delimiter = listDelimiter;
        text = listText;
    }

    int count() {
        int cnt = 1;
        for (int i = 0; i < text.length(); i++)
            if (text[i]==delimiter) cnt++;
        return cnt;
    }

    int add(String strLine) {
        if (text!="") text+=delimiter;
        text += strLine;
        return count()-1;
    }

    void replace(int lineNum, String strLine) {
        int cnt = 0;
        int pos=0;
        for (int i = 0; i < text.length(); i++) if (text[i]==delimiter) {
            cnt++;
            if (cnt>lineNum) {
                text=text.substring(0,pos)+strLine+text.substring(i);
                return;
            }
            pos=i+1;
        }
        if (lineNum>cnt) {
            for (int i = cnt; i < lineNum; i++)
                text += delimiter; // дополним строки разделителями до заданной
            text +=strLine;
        } else {
            text=text.substring(0,pos)+strLine;
        }
    }

    bool del(int lineNum) {
        int cnt = 0;
        int pos=0;
        for (int i = 0; i < text.length(); i++) if (text[i]==delimiter) {
            cnt++;
            if (cnt>lineNum) {
                text=text.substring(0,pos)+text.substring(i+1);
                return true;
            }
            pos=i+1;
        }
        if (cnt>=lineNum) {
            if (pos>0) pos--;
            text=text.substring(0,pos);
            return true;
        }
        return false;
    }

    String getLine(int lineNum) {
        int cnt = 0;
        int pos=0;
        for (int i = 0; i < text.length(); i++) if (text[i]==delimiter) {
            cnt++;
            if (cnt>lineNum) {
                return text.substring(pos,i);
            }
            pos=i+1;
        }
        if (cnt>=lineNum) return text.substring(pos);
        return "";
    }

    int getLineInt(int lineNum) {
        String strLine = getLine(lineNum);
        strLine.trim();
        if (strLine=="" || strLine[0]<'0' || strLine[0]>'9') return 0;
        return strLine.toInt();
    }

    double getLineFloat(int lineNum) {
        String strLine = getLine(lineNum);
        strLine.trim();
        if (strLine=="" || strLine[0]<'0' || strLine[0]>'9') return 0;
        return strLine.toDouble();
    }

};



#endif //STRINGLIST_H
