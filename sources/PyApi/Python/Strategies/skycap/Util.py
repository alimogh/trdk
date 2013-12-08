

def updateContext(strategy):
    context = strategy.context
    params = context.params
    if hasattr(params, "startCashBalance") is True:
        return
    cash = context.tradeSystem.cashBalance
    if cash <= 0:
        raise Exception(
            "Can't start with negative or empty account: {0}".format(cash))
    params.startCashBalance = str(cash)
    if hasattr(params, "cash_balance") is False:
        context.params.cash_balance = str(cash)


def getCurrentVolume(strategy):
    context = strategy.context
    params = strategy.context.params
    return float(params.startCashBalance) - context.tradeSystem.cashBalance


def getFullBalance(strategy):
    return float(strategy.context.params.cash_balance)


def checkAccount(strategy, tradeVol):

    context = strategy.context
    params = context.params
    tradeSystem = context.tradeSystem

    if tradeSystem.excessLiquidity <= float(params.min_excess_liquidity):
        strategy.log.warn(
            'Can\'t open position: Params::min_excess_liquidity reached:'
            ' {0} <= {1}.'
            .format(tradeSystem.excessLiquidity, params.min_excess_liquidity))
        return False

    maxAllowedAccVol = float(params.max_allocated_account_volume)
    maxAccVol = getFullBalance(strategy) * maxAllowedAccVol
    currVol = getCurrentVolume(strategy)
    newAccVol = currVol + tradeVol

    if newAccVol >= maxAccVol:
        strategy.log.warn(
            'Can\'t open position: Params::max_allocated_volume reached:'
            ' {0} ({1} + {2}) >= {3} ({4} * {5})).'
            .format(
                newAccVol, currVol, tradeVol,
                maxAccVol, getFullBalance(strategy), maxAllowedAccVol))
        return False

    return True


def isActualForClosingPositionUpdate(position):
    return \
        position.isClosed is False \
        and position.isOpened is True \
        and position.hasActiveCloseOrders is False
