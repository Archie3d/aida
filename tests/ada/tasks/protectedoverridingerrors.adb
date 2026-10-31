procedure ProtectedOverridingErrors is
    protected Guard is
        overriding procedure Act;
        overriding entry Open;
    end Guard;
    protected body Guard is
        procedure Act is
        begin
            null;
        end Act;
        entry Open when True is
        begin
            null;
        end Open;
    end Guard;
begin
    null;
end ProtectedOverridingErrors;
