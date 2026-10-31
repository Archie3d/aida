package body GenericLimitedArrayLibrary is
    function Valid return Boolean is
    begin
        return Value'First = 3 and Value'Length = 3
            and Value (3).Self.all'Address = Value (3).Value'Address;
    end Valid;
end GenericLimitedArrayLibrary;
