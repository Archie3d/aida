package body GenericLimitedLibrary is
    function Test return Boolean is
    begin
        return Valid (Value);
    end Test;
    function Test_Other return Boolean is
    begin
        return Valid (Other);
    end Test_Other;
end GenericLimitedLibrary;
