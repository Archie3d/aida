procedure ReservedConstructErrors is
    Value : aliased Integer;
    protected Lock is
    end Lock;
    protected type Lock_Type is
    end Lock_Type;
    type Callback is access protected procedure;
    type Plain is interface;
    type Limited_Interface is limited interface;
    type Task_Interface is task interface;
    type Protected_Interface is protected interface;
    type Synchronized_Interface is synchronized interface;
begin
    requeue Work;
    requeue Work with abort;
    delay until 0;
end ReservedConstructErrors;
