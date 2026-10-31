procedure ProtectedNullErrors is
    protected Guard is
        procedure Act;
    end Guard;
    protected body Guard is
        procedure Act is null;
    end Guard;
    protected Abstract_Guard is
        procedure Act is abstract;
    end Abstract_Guard;
begin
    null;
end ProtectedNullErrors;
