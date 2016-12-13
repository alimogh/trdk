
Sub FormatIntradayTrendStrategyReport()

	Rows("1:1").Select
	Selection.Font.Bold = True
	Cells.Select
	Selection.AutoFilter
	Selection.Columns.AutoFit
	Range("B2").Select
	ActiveWindow.FreezePanes = True
	Columns("I:I").Select
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
	Selection.FormatConditions.AddColorScale ColorScaleType:=3
	Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
	Selection.FormatConditions(1).ColorScaleCriteria(1).Type = _
		xlConditionValueLowestValue
	With Selection.FormatConditions(1).ColorScaleCriteria(1).FormatColor
		.Color = 7039480
		.TintAndShade = 0
	End With
	Selection.FormatConditions(1).ColorScaleCriteria(2).Type = _
		xlConditionValuePercentile
	Selection.FormatConditions(1).ColorScaleCriteria(2).Value = 50
	With Selection.FormatConditions(1).ColorScaleCriteria(2).FormatColor
		.Color = 8711167
		.TintAndShade = 0
	End With
	Selection.FormatConditions(1).ColorScaleCriteria(3).Type = _
		xlConditionValueHighestValue
	With Selection.FormatConditions(1).ColorScaleCriteria(3).FormatColor
		.Color = 8109667
		.TintAndShade = 0
	End With

	Columns("K:K").Select
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
	Selection.FormatConditions.AddColorScale ColorScaleType:=3
	Selection.FormatConditions(Selection.FormatConditions.Count).SetFirstPriority
	Selection.FormatConditions(1).ColorScaleCriteria(1).Type = _
		xlConditionValueLowestValue
	With Selection.FormatConditions(1).ColorScaleCriteria(1).FormatColor
		.Color = 7039480
		.TintAndShade = 0
	End With
	Selection.FormatConditions(1).ColorScaleCriteria(2).Type = _
		xlConditionValuePercentile
	Selection.FormatConditions(1).ColorScaleCriteria(2).Value = 50
	With Selection.FormatConditions(1).ColorScaleCriteria(2).FormatColor
		.Color = 8711167
		.TintAndShade = 0
	End With
	Selection.FormatConditions(1).ColorScaleCriteria(3).Type = _
		xlConditionValueHighestValue
	With Selection.FormatConditions(1).ColorScaleCriteria(3).FormatColor
		.Color = 8109667
		.TintAndShade = 0
	End With

	' P&L vol
	rowOfTotals = Cells.SpecialCells(xlLastCell).Row + 1
	Range("I" & rowOfTotals).Select
	Selection.FormatConditions.Delete
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(I2:I" & (rowOfTotals - 1) & ")"

	' P&L %
	Range("J" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=AVERAGE(J2:J" & (rowOfTotals - 1) & ")"
	Selection.NumberFormat = "0.00000"

	' Number of wins
	Range("L" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(L2:L" & (rowOfTotals - 1) & ")"
	' Number of losses
	Range("M" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(M2:M" & (rowOfTotals - 1) & ")"
	' Number of operations
	rowOfTotals = rowOfTotals + 1
	Range("M" & rowOfTotals).Select
	Selection.Font.Bold = True
	ActiveCell.Formula = "=SUM(M" & (rowOfTotals - 1) & ":L" & (rowOfTotals - 1) & ")"

	' % of wins
	rowOfTotals = rowOfTotals + 1
	Range("L" & rowOfTotals).Select
	Selection.Font.Bold = True
	Selection.NumberFormat = "0"
	ActiveCell.Formula = "=(L" & (rowOfTotals - 2) & "/M" & (rowOfTotals - 1) & ") * 100"
	' % of losses
	Range("M" & rowOfTotals).Select
	Selection.Font.Bold = True
	Selection.NumberFormat = "0"
	ActiveCell.Formula = "=(M" & (rowOfTotals - 2) & "/M" & (rowOfTotals - 1) & ") * 100"

End Sub

