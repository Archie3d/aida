procedure InterfaceCategoryErrors is
    package Model is
        type Ordinary is interface;
        type Limited_Root is limited interface;
        type Bad_Limited is limited interface and Ordinary;
        type Sync is synchronized interface;
        type Task_Root is task interface;
        type Protected_Root is protected interface;
        type Wrong_Task is synchronized interface and Task_Root;
        type Wrong_Protected is task interface and Protected_Root;
        type Both is task interface and Task_Root and Protected_Root;
        type Record_Sync is new Sync with null record;
        type Record_Task is new Task_Root with null record;
        type Record_Protected is new Protected_Root with null record;
        type Limited_Record is tagged limited null record;
        type Nonlimited_Contract is new Limited_Record and Ordinary with null record;
    end Model;
begin
    null;
end InterfaceCategoryErrors;
