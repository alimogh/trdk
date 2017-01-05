
Sub FormatIntradayMaTrendStrategyReport()

    Rows("1:1").Select
    Selection.Font.Bold = True
    Cells.Select
    Selection.AutoFilter
    Selection.Columns.AutoFit
	Range("C2").Select
	ActiveWindow.FreezePanes = True

	Columns("J:J").Select
	Selection.FormatConditions.AddDatabar
    Selection.FormatConditions(Selection.FormatConditions.Count).ShowValue = True
    Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
    With Selection.FormatConditions(1)
        .MinPoint.Modify newtype:=xlConditionValueAutomaticMin
        .MaxPoint.Modify newtype:=xlConditionValueAutomaticMax
    End With
    With Selection.FormatConditions(1).BarColor
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).BarFillType = xlDataBarFillGradient
    Selection.FormatConditions(1).Direction = xlContext
    Selection.FormatConditions(1).NegativeBarFormat.ColorType = xlDataBarColor
    Selection.FormatConditions(1).BarBorder.Type = xlDataBarBorderSolid
    Selection.FormatConditions(1).NegativeBarFormat.BorderColorType = _
        xlDataBarColor
    With Selection.FormatConditions(1).BarBorder.Color
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).AxisPosition = xlDataBarAxisAutomatic
    With Selection.FormatConditions(1).AxisColor
        .Color = 0
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.Color
        .Color = 255
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.BorderColor
        .Color = 255
        .TintAndShade = 0
    End With

	Columns("L:L").Select
	Selection.FormatConditions.AddDatabar
    Selection.FormatConditions(Selection.FormatConditions.Count).ShowValue = True
    Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
    With Selection.FormatConditions(1)
        .MinPoint.Modify newtype:=xlConditionValueAutomaticMin
        .MaxPoint.Modify newtype:=xlConditionValueAutomaticMax
    End With
    With Selection.FormatConditions(1).BarColor
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).BarFillType = xlDataBarFillGradient
    Selection.FormatConditions(1).Direction = xlContext
    Selection.FormatConditions(1).NegativeBarFormat.ColorType = xlDataBarColor
    Selection.FormatConditions(1).BarBorder.Type = xlDataBarBorderSolid
    Selection.FormatConditions(1).NegativeBarFormat.BorderColorType = _
        xlDataBarColor
    With Selection.FormatConditions(1).BarBorder.Color
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).AxisPosition = xlDataBarAxisAutomatic
    With Selection.FormatConditions(1).AxisColor
        .Color = 0
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.Color
        .Color = 255
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.BorderColor
        .Color = 255
        .TintAndShade = 0
    End With

	' Unrealized PnL
	Columns("M:M").Select
	Selection.FormatConditions.AddDatabar
    Selection.FormatConditions(Selection.FormatConditions.Count).ShowValue = True
    Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
    With Selection.FormatConditions(1)
        .MinPoint.Modify newtype:=xlConditionValueAutomaticMin
        .MaxPoint.Modify newtype:=xlConditionValueAutomaticMax
    End With
    With Selection.FormatConditions(1).BarColor
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).BarFillType = xlDataBarFillGradient
    Selection.FormatConditions(1).Direction = xlContext
    Selection.FormatConditions(1).NegativeBarFormat.ColorType = xlDataBarColor
    Selection.FormatConditions(1).BarBorder.Type = xlDataBarBorderSolid
    Selection.FormatConditions(1).NegativeBarFormat.BorderColorType = _
        xlDataBarColor
    With Selection.FormatConditions(1).BarBorder.Color
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).AxisPosition = xlDataBarAxisAutomatic
    With Selection.FormatConditions(1).AxisColor
        .Color = 0
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.Color
        .Color = 255
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.BorderColor
        .Color = 255
        .TintAndShade = 0
    End With
	Columns("N:N").Select
	Selection.FormatConditions.AddDatabar
    Selection.FormatConditions(Selection.FormatConditions.Count).ShowValue = True
    Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
    With Selection.FormatConditions(1)
        .MinPoint.Modify newtype:=xlConditionValueAutomaticMin
        .MaxPoint.Modify newtype:=xlConditionValueAutomaticMax
    End With
    With Selection.FormatConditions(1).BarColor
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).BarFillType = xlDataBarFillGradient
    Selection.FormatConditions(1).Direction = xlContext
    Selection.FormatConditions(1).NegativeBarFormat.ColorType = xlDataBarColor
    Selection.FormatConditions(1).BarBorder.Type = xlDataBarBorderSolid
    Selection.FormatConditions(1).NegativeBarFormat.BorderColorType = _
        xlDataBarColor
    With Selection.FormatConditions(1).BarBorder.Color
        .Color = 8700771
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).AxisPosition = xlDataBarAxisAutomatic
    With Selection.FormatConditions(1).AxisColor
        .Color = 0
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.Color
        .Color = 255
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.BorderColor
        .Color = 255
        .TintAndShade = 0
    End With
	Columns("O:O").Select
	Selection.FormatConditions.AddDatabar
    Selection.FormatConditions(Selection.FormatConditions.Count).ShowValue = True
    Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
    With Selection.FormatConditions(1)
        .MinPoint.Modify newtype:=xlConditionValueAutomaticMin
        .MaxPoint.Modify newtype:=xlConditionValueAutomaticMax
    End With
    With Selection.FormatConditions(1).BarColor
        .Color = 5920255
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).BarFillType = xlDataBarFillGradient
    Selection.FormatConditions(1).Direction = xlContext
    Selection.FormatConditions(1).NegativeBarFormat.ColorType = xlDataBarColor
    Selection.FormatConditions(1).BarBorder.Type = xlDataBarBorderSolid
    Selection.FormatConditions(1).NegativeBarFormat.BorderColorType = _
        xlDataBarColor
    With Selection.FormatConditions(1).BarBorder.Color
        .Color = 5920255
        .TintAndShade = 0
    End With
    Selection.FormatConditions(1).AxisPosition = xlDataBarAxisAutomatic
    With Selection.FormatConditions(1).AxisColor
        .Color = 0
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.Color
        .Color = 255
        .TintAndShade = 0
    End With
    With Selection.FormatConditions(1).NegativeBarFormat.BorderColor
        .Color = 255
        .TintAndShade = 0
    End With

    ' P&L vol
    rowOfTotals = Cells.SpecialCells(xlLastCell).Row + 1
	Range("J" & rowOfTotals).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(J2:J" & (rowOfTotals - 1) & ")"

	' P&L %
	Range("K" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=AVERAGE(K2:K" & (rowOfTotals - 1) & ")"
	Selection.NumberFormat = "0.00000"

	' Unrealized PnL
	Range("M" & rowOfTotals).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(M2:M" & (rowOfTotals - 1) & ")"
	Range("M" & (rowOfTotals + 1)).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=AVERAGE(M2:M" & (rowOfTotals - 1) & ")"
	' Unrealized profit
	Range("N" & rowOfTotals).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(N2:N" & (rowOfTotals - 1) & ")"
	Range("N" & (rowOfTotals + 1)).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=AVERAGE(N2:N" & (rowOfTotals - 1) & ")"
	' Unrealized loss
	Range("O" & rowOfTotals).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(O2:O" & (rowOfTotals - 1) & ")"
	Range("O" & (rowOfTotals + 1)).Select
	Selection.FormatConditions.Delete
    Selection.Font.Bold = True
	ActiveCell.Formula = "=AVERAGE(O2:O" & (rowOfTotals - 1) & ")"

	' Number of wins
	Range("P" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(P2:P" & (rowOfTotals - 1) & ")"
	' Number of losses
	Range("Q" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(Q2:Q" & (rowOfTotals - 1) & ")"
	' Number of operations
	rowOfTotals = rowOfTotals + 1
	Range("Q" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(Q" & (rowOfTotals - 1) & ":P" & (rowOfTotals - 1) & ")"

	' % of wins
	rowOfTotals = rowOfTotals + 1
	Range("P" & rowOfTotals).Select
	Selection.Font.Bold = True
    Selection.NumberFormat = "0"
	ActiveCell.Formula = "=(P" & (rowOfTotals - 2) & "/Q" & (rowOfTotals - 1) & ") * 100"
	' % of losses
	Range("Q" & rowOfTotals).Select
	Selection.Font.Bold = True
    Selection.NumberFormat = "0"
	ActiveCell.Formula = "=(Q" & (rowOfTotals - 2) & "/Q" & (rowOfTotals - 1) & ") * 100"

End Sub
