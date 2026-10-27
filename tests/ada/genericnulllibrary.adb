package body GenericNullLibrary is
    procedure Invoke (Object : in out T) is
    begin
        Hook (Object => Object);
    end Invoke;
    package body Inner is
        procedure Invoke (Object : in out T) is
        begin
            Action (Object => Object);
        end Invoke;
    end Inner;
end GenericNullLibrary;
