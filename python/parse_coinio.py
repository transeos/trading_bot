#!/usr/bin/python3

# -*- Python -*-

#*****************************************************************
#
# WARRANTY:
# Use all material in this file at your own risk.
#
# Created by Hiranmoy Basak on 04/01/18.
#

import os
import datetime

from enum import Enum


# ============================== globals =======================
gTraderHome = "."
gExchangeFile = "data/coinio_exchanges.json"
gCurrencyFile = "data/coinio_currencies.json"
gExchangeEnumFile = "lib/include/ExchangeEnums.h"
gCurrencyEnumFile = "lib/include/CurrencyEnums.h"


# ====================== ASCII escape char ======================
class color(Enum):
  cRed = '\x1b[6;30;41m'
  cGreen = '\x1b[6;30;42m'
  cYellow = '\x1b[6;30;43m'
  cBlue = '\x1b[6;30;44m'
  cPink = '\x1b[6;30;45m'
  cCyan = '\x1b[6;30;46m'
  cWhite = '\x1b[6;30;47m'
  cEnd = '\x1b[0m'


# ============================ functions ==========================
def DumpActivity(dumpStr, colorCode):
  print(colorCode.value + dumpStr + color.cEnd.value)

def GetExchangeFile():
	return gTraderHome + "/" + gExchangeFile

def GetCurrencyFile():
	return gTraderHome + "/" + gCurrencyFile

def GetExchangeEnumFile():
	return gTraderHome + "/" + gExchangeEnumFile

def GetCurrencyEnumFile():
	return gTraderHome + "/" + gCurrencyEnumFile

def PrintEnums(pEnumFile, enumList, strPrint = False):
	for idx in range(len(enumList)):
		if (strPrint):
			pEnumFile.write("  %s%s%s" % ('"', enumList[idx], '"'))
		else:
			pEnumFile.write("  " + enumList[idx])

			if (idx == 0):
				pEnumFile.write(" = 0")

		if (idx < (len(enumList) - 1)):
			pEnumFile.write(",")

		pEnumFile.write("\n")


def PrintHeader(fileName):
	pEnumFile = open(fileName, 'a')
	pEnumFile.write("//                    Copyright " + str(datetime.datetime.now().year) + "\n")
	pEnumFile.write("//\n")
	pEnumFile.write("//                  All Rights Reserved.\n")
	pEnumFile.write("//\n")
	pEnumFile.write("//           THIS WORK CONTAINS TRADE SECRET And\n")
	pEnumFile.write("//       PROPRIETARY INFORMATION WHICH Is THE PROPERTY\n")
	pEnumFile.write("//                  OF ITS LICENSOR\n")
	pEnumFile.write("//            AND IS SUBJECT TO LICENSE TERMS.\n")
	pEnumFile.write("//\n")
	pEnumFile.write("//*****************************************************************/\n")
	pEnumFile.write("//\n")
	pEnumFile.write("// No part of this file may be reproduced, stored in a retrieval system,\n")
	pEnumFile.write("// Or transmitted in any form Or by any means --- electronic, mechanical,\n")
	pEnumFile.write("// photocopying, recording, Or otherwise --- without prior written permission\n")
	pEnumFile.write("//\n")
	pEnumFile.write("// WARRANTY:\n")
	pEnumFile.write("// Use all material in this file at your own risk.\n")
	pEnumFile.write("//\n")
	pEnumFile.write("// This file is auto-generated by python script on " + str(datetime.datetime.now()) + "\n")
	pEnumFile.write("//\n")
	pEnumFile.write("\n")

	pEnumFile.close()


def PrintExchangeHeader():
	pExchangeEnumFile = open(GetExchangeEnumFile(), 'a')

	pExchangeEnumFile.write("#ifndef EXCHANGE_ENUMS_H\n")
	pExchangeEnumFile.write("#define EXCHANGE_ENUMS_H\n")

	pExchangeEnumFile.close()


def PrintCurrencyHeader():
	pCurrencyEnumFile = open(GetCurrencyEnumFile(), 'a')

	pCurrencyEnumFile.write("#ifndef CURRENCY_ENUMS_H\n")
	pCurrencyEnumFile.write("#define CURRENCY_ENUMS_H\n")

	pCurrencyEnumFile.close()


def PrintCPPHheader(fileName):
	pEnumFile = open(fileName, 'a')

	pEnumFile.write("\n")
	pEnumFile.write("\n")
	pEnumFile.write("#include <iostream>\n")
	pEnumFile.write("#include <algorithm>\n")
	pEnumFile.write("#include <cassert>\n")
	pEnumFile.write("\n")

	pEnumFile.close()


def PrintExchangeFooter():
	pExchangeEnumFile = open(GetExchangeEnumFile(), 'a')

	pExchangeEnumFile.write("#endif //EXCHANGE_ENUMS_H\n")

	pExchangeEnumFile.close()


def PrintCurrencyFooter():
	pCurrencyEnumFile= open(GetCurrencyEnumFile(), 'a')

	pCurrencyEnumFile.write("#endif //CURRENCY_ENUMS_H\n")

	pCurrencyEnumFile.close()


def PrintExchanges(exchangeEnumList, exchangeStrList):
	pExchangeEnumFile = open(GetExchangeEnumFile(), 'a')

	pExchangeEnumFile.write("enum class exchange_t\n")
	pExchangeEnumFile.write("{\n")
	pExchangeEnumFile.write("  NOEXCHANGE = -2,\n")
	pExchangeEnumFile.write("  COINMARKETCAP = -1,\n")

	PrintEnums(pExchangeEnumFile, exchangeEnumList);

	pExchangeEnumFile.write("};\n")
	pExchangeEnumFile.write("\n")
	pExchangeEnumFile.write("static std::vector<std::string> g_exchange_strs = {\n")
	pExchangeEnumFile.write("  %sNOEXCHANGE%s,\n" % ('"','"'))
	pExchangeEnumFile.write("  %sCOINMARKETCAP%s,\n" % ('"','"'))

	PrintEnums(pExchangeEnumFile, exchangeStrList, True);

	pExchangeEnumFile.write("};\n")
	pExchangeEnumFile.write("\n")

	pExchangeEnumFile.close()


def PrintCurrencies(currencyEnumList, currencyStrList):
	pCurrencyEnumFile = open(GetCurrencyEnumFile(), 'a')

	pCurrencyEnumFile.write("enum class currency_t\n")
	pCurrencyEnumFile.write("{\n")
	pCurrencyEnumFile.write("  UNDEF = -1,\n")

	PrintEnums(pCurrencyEnumFile, currencyEnumList);

	pCurrencyEnumFile.write("};\n")
	pCurrencyEnumFile.write("\n")
	pCurrencyEnumFile.write("static std::vector<std::string> g_currency_strs = {\n")
	pCurrencyEnumFile.write("  %sUNDEF%s,\n" % ('"','"'))

	PrintEnums(pCurrencyEnumFile, currencyStrList, True);

	pCurrencyEnumFile.write("};\n")
	pCurrencyEnumFile.write("\n")

	pCurrencyEnumFile.close()


def CheckForTraderHome():
	global gTraderHome

	if (os.environ.get('TRADER_HOME') == None):
		DumpActivity("TRADER_HOME not defined", color.cRed)
		exit(1)

	gTraderHome = os.environ['TRADER_HOME']


# ============================== run ==============================
CheckForTraderHome()

if (os.path.isfile(GetExchangeFile()) == 0):
	DumpActivity(GetExchangeFile() + " file not found", color.cRed)
	exit(1)

# "-" is replaced by "_"
command1 = "\grep exchange_id " + GetExchangeFile() + " | sed 's/.*: .//g' | sed 's/.,.*//g' | sed 's/-/_/g' > tmp1"
os.system(command1)
command2 = "\grep exchange_id " + GetExchangeFile() + " | sed 's/.*: .//g' | sed 's/.,.*//g' > tmp2"
os.system(command2)

exchangeEnumList = []
exchangeStrList = []

pExchangeListFile1 = open("tmp1", 'r')
pExchangeListFile2 = open("tmp2", 'r')

for line in pExchangeListFile1:
	exchangeEnumList.append(line.strip())
for line in pExchangeListFile2:
	exchangeStrList.append(line.strip())

pExchangeListFile1.close()
pExchangeListFile2.close()
os.remove("tmp1")
os.remove("tmp2")

if (os.path.isfile(GetExchangeEnumFile())):
	os.remove(GetExchangeEnumFile())

PrintHeader(GetExchangeEnumFile())

PrintExchangeHeader()

PrintCPPHheader(GetExchangeEnumFile())

PrintExchanges(exchangeEnumList, exchangeStrList)

PrintExchangeFooter()


if (os.path.isfile(GetCurrencyFile()) == 0):
	DumpActivity(GetCurrencyFile() + " file not found", color.cRed)
	exit(1)

# "@" is replaced by "_"
command1 = "\grep asset_id " + GetCurrencyFile() + " | sed 's/.*: .//g' | sed 's/.,.*//g' | sed 's/@/_/g' > tmp1"
os.system(command1)
command2 = "\grep asset_id " + GetCurrencyFile() + " | sed 's/.*: .//g' | sed 's/.,.*//g' > tmp2"
os.system(command2)

currencyEnumList = []
currencyStrList = []

pCurrencyListFile1 = open("tmp1", 'r')
pCurrencyListFile2 = open("tmp2", 'r')

for line in pCurrencyListFile1:
	# if currency starts with a number, "NUM_" prefix is added
	currencyStr = line.strip()
	if (currencyStr[0].isdigit()):
		currencyStr = "_" + currencyStr

	currencyEnumList.append(currencyStr)
for line in pCurrencyListFile2:
	currencyStrList.append(line.strip())

pCurrencyListFile1.close()
pCurrencyListFile2.close()
os.remove("tmp1")
os.remove("tmp2")


if (os.path.isfile(GetCurrencyEnumFile())):
	os.remove(GetCurrencyEnumFile())

PrintHeader(GetCurrencyEnumFile())

PrintCurrencyHeader()

PrintCPPHheader(GetCurrencyEnumFile())

PrintCurrencies(currencyEnumList, currencyStrList)

PrintCurrencyFooter()

exit(0)
