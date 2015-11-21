
import SocketServer
from protobuf_to_dict import protobuf_to_dict
from trdk.EngineService.MarketData_pb2 import *
from trdk.EngineService.DropCopy_pb2 import *
import struct
import uuid


class TestServerHandler(SocketServer.BaseRequestHandler):

    def __init__(self, request, client_address, server):

        self._securities = dict()
        self._orders = dict()
        self._handlers = {
            ServiceData.TYPE_KEEP_ALIVE: self._handle_keep_alive,
            ServiceData.TYPE_DICTIONARY: self._handle_dictionary,
            ServiceData.TYPE_BOOK_UPDATE: self._handle_book_update,
            ServiceData.TYPE_ORDER: self._handle_type_order,
            ServiceData.TYPE_TRADE: self._handle_type_trade,
        }

        SocketServer.BaseRequestHandler.__init__(self, request, client_address, server)

    def setup(self):
        print "Connected from {0}:{1}.".format(self.client_address[0], self.client_address[1])
        return SocketServer.BaseRequestHandler.setup(self)

    def handle(self):

        while True:

            size_packet = self.request.recv(4)
            if size_packet is False:
                print "Connection closed."
                break

            size_packet = struct.unpack("I", size_packet)[0]
            message = self.request.recv(size_packet)
            data = ServiceData()
            data.ParseFromString(message)

            if data.type in self._handlers:
                try:
                    self._handlers[data.type](data)
                except Exception, ex:
                    print "Error processing incoming message: \"{0}\".".format(ex)
            else:
                print "Unknown message received: \"{0}\".".format(data)

    def finish(self):
        print "Disconnected from {0}:{1}.".format(self.client_address[0], self.client_address[1])
        return SocketServer.BaseRequestHandler.finish(self)

    def _handle_keep_alive(self, message):
        print "MESSAGE: keep alive"
        pass

    def _handle_dictionary(self, message):
        print "MESSAGE: dictionary"
        for record in message.dictionary.securities:
            if record.security.product == Security.PRODUCT_SPOT:
                product = "spot"
            else:
                product = "<UNKNOWN>"
            if record.security.type == Security.TYPE_FOR:
                security_type = "Foreign Exchange Contract"
            else:
                security_type = "<UNKNOWN>"
            print "\tSecurity #{0}:\t \"{1}\" from \"{2}\"; Product: {3}; Type: {4};".format(
                record.id,
                record.security.symbol,
                record.security.source,
                product,
                security_type
            )
            assert record.id not in self._securities
            self._securities[record.id] = record.security

    def _handle_book_update(self, message):
        print "MESSAGE: book update"

    def _handle_type_order(self, message):

        print "MESSAGE: order"
        order = message.order

        order_id = str(uuid.UUID(bytes=order.id.data)).upper()

        trade_system_order_id = "<NULL>"
        if order.HasField("trade_system_order_id"):
            trade_system_order_id = order.trade_system_order_id

        if order.status == ORDER_STATUS_SENT:
            status = "sent"
        elif order.status == ORDER_STATUS_ACTIVE:
            status = "active"
        elif order.status == ORDER_STATUS_FILLED:
            status = "filled"
        elif order.status == ORDER_STATUS_FILLED_PARTIALLY:
            status = "filled partially"
        elif order.status == ORDER_STATUS_ERROR:
            status = "error"
        elif order.status == ORDER_STATUS_REQUESTED_CANCEL:
            status = "cancel requested"
        elif order.status == ORDER_STATUS_CANCELED:
            status = "canceled"
        else:
            status = "<UNKNOWN>"

        order_time = "<NULL>"
        if order.HasField("order_time"):
            order_time = order.order_time

        execution_time = "<NULL>"
        if order.HasField("execution_time"):
            execution_time = order.execution_time

        counter_amount = "<NULL>"
        if order.HasField("counter_amount"):
            counter_amount = order.counter_amount
        last_trade_id = "<NULL>"
        if order.HasField("last_trade_id"):
            last_trade_id = order.last_trade_id

        print "\tID: \"{0}\" / \"{1}\"; Strategy: \"{2}\"; Mode: \"{3}\";".format(
            order_id,
            trade_system_order_id,
            str(uuid.UUID(bytes=order.strategy_id.data)).upper(),
            order.mode)
        self._print_order_parameters(order.params)
        print "\tStatus: {0}; Order time: \"{1}\"; Exec. time: \"{2}\";".format(status, order_time, execution_time)
        print "\tLast trade ID: \"{0}\"; Number of trades: \"{1}\";".format(last_trade_id, order.number_of_trades)
        print "\tAvg. trade price: \"{0}\"; Executed qty: \"{1}\"; Counter amount: \"{2}\";".format(
            order.avg_trade_price,
            order.executed_qty,
            counter_amount)
        self._print_top_of_book(order)

        self._orders[order_id] = order

    def _handle_type_trade(self, message):

        print "MESSAGE: trade"
        trade = message.trade

        order_id = str(uuid.UUID(bytes=trade.order_id.data)).upper()

        if order_id in self._orders:
            know_order = "OK!"
        else:
            know_order = "<UNKNOWN>"

        print "\tTime: \"{0}\"; ID \"{1}\"; Strategy: \"{2}\"; Mode: \"{3}\";".format(
            trade.time,
            trade.id,
            str(uuid.UUID(bytes=trade.strategy_id.data)).upper(),
            trade.mode)
        print "\tIs maker: \"{0}\"; Price: \"{1}\"; Qty \"{2}\"; Counter amount: \"{3}\";".format(
            trade.is_maker,
            trade.price,
            trade.qty,
            trade.counter_amount)
        print "\tOrder ID: \"{0}\" ({1}); TS Order ID: \"{2}\";".format(
            order_id,
            know_order,
            trade.trade_system_order_id)
        self._print_order_parameters(trade.order_params)
        self._print_top_of_book(trade)

    def _print_order_parameters(self, params):

        order_type = "<NULL>"
        if params.HasField("type"):
            if params.type == OrderParameters.TYPE_LMT:
                order_type = "limit"
            elif params.type == OrderParameters.TYPE_MKT:
                order_type = "market"
            else:
                order_type = "<UNKNOWN>"

        if params.security_id in self._securities:
            security = "{0} from {1}".format(
                self._securities[params.security_id].symbol,
                self._securities[params.security_id].source)
        else:
            security = "<UNKNOWN> with ID \"{0}\"".format(params.security_id)

        if params.side == OrderParameters.SIDE_BUY:
            side = "buy"
        elif params.side == OrderParameters.SIDE_SELL:
            side = "sell"
        else:
            side = "<UNKNOWN>"

        price = "<NULL>"
        if params.HasField("price"):
            price = params.price

        time_in_force = "<NULL>"
        if params.HasField("time_in_force"):
            if params.time_in_force == OrderParameters.TIME_IN_FORCE_DAY:
                time_in_force = "Good Till Day"
            elif params.time_in_force == OrderParameters.TIME_IN_FORCE_GTC:
                time_in_force = "Good Till Cancel"
            elif params.time_in_force == OrderParameters.TIME_IN_FORCE_OPG:
                time_in_force = "At the Opening"
            elif params.time_in_force == OrderParameters.TIME_IN_FORCE_IOC:
                time_in_force = "Immediate or Cancel"
            elif params.time_in_force == OrderParameters.TIME_IN_FORCE_FOK:
                time_in_force = "Fill or Kill"
            else:
                time_in_force = "<UNKNOWN>"

        min_qty = "<NULL>"
        if params.HasField("min_qty"):
            min_qty = params.min_qty

        user = "<NULL>"
        if params.HasField("user"):
            user = params.user

        print "\tType: {0}; Security: {1}; Side: {2}; Qty: \"{3}\"; Price: \"{4}\";".format(
            order_type,
            security,
            side,
            params.qty,
            price)
        print "\tTime in Force: {0}; Currency: \"{1}\"; Min. qty: \"{2}\"; User: \"{3}\";".format(
            time_in_force,
            params.currency,
            min_qty,
            user)

    def _print_top_of_book(self, message):
        if message.HasField("top_of_book"):
            print "\tTop of Book: bid {0}/{1}; ask {2}/3;".format(
                message.top_of_book.bid.price,
                message.top_of_book.bid.qty,
                message.top_of_book.ask.price,
                message.top_of_book.ask.qty)
        else:
            print "\tTop of Book: <NULL>;".format(message)

if __name__ == "__main__":
    server = SocketServer.TCPServer(("192.168.132.32", 5689), TestServerHandler)
    server.serve_forever()
