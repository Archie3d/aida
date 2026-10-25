procedure ProtectedIndicatorErrors is
    protected Guard is
        not overriding procedure Act;
        not overriding entry Open;
    end Guard;
    protected body Guard is
        not overriding procedure Act is
        begin
            null;
        end Act;
        overriding entry Open when True is
        begin
            null;
        end Open;
    end Guard;
begin
    null;
end ProtectedIndicatorErrors;
