const DesignerPanel = withStyles(designStyle)(props => {
    const { classes, open, addNotification } = props;
    const [ effects, setEffects ] = useState(undefined);
    const [ abortControler, setAbortControler ] = useState(undefined);
    const [ nextRefreshDate, setNextRefreshDate] = useState(undefined);
    const [ editing, setEditing ] = useState(false);
    const [ requestRunning, setRequestRunning ] = useState(false);
    const [ pendingInterval, setPendingInterval ] = useState(effects && effects.effectInterval);

    useEffect(() => {
        if (abortControler) {
            abortControler.abort();
        }

        if (open) {
            const aborter = new AbortController();
            setAbortControler(aborter);

            const timer = setTimeout(()=>{
                setNextRefreshDate(Date.now());
            },3000);
    
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/getEffectList`,{signal:aborter.signal})
                .then(resp => resp.json())
                .then(setEffects)
                .then(()=>clearTimeout(timer))
                .catch(err => addNotification("Error","Service","Get Effect List",err));
    
            return () => {
                abortControler && abortControler.abort();
                clearTimeout(timer);
            }
        }
    },[open,nextRefreshDate]);

    const requestRefresh = () => setTimeout(()=>setNextRefreshDate(Date.now()),50);

    const chipRequest = (url,options,operation) => 
        new Promise((resolve,reject) => 
            fetch(url,options)
                .then(resolve)
                .catch(err => {addNotification("Error",operation,err);reject(err)}));

    const navigateTo = (idx)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/setCurrentEffectIndex?`,{method:"POST", body: new URLSearchParams({currentEffectIndex:idx})}, "navigateTo")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    };

    const effectEnable = (idx,enable)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${enable?"enable":"disable"}Effect`,{method:"POST", body:new URLSearchParams({effectIndex:idx})},"effectEnable")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    };

    const navigate = (up)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/${up ? "nextEffect" : "previousEffect"}`,{method:"POST"},"nvigate")
                .then(resolve)
                .then(requestRefresh)
                .catch(reject)
                .finally(()=>setRequestRunning(false));    
        });
    };

    const updateEventInterval = (interval)=>{
        return new Promise((resolve,reject)=>{
            setRequestRunning(true);
            chipRequest(`${httpPrefix !== undefined ? httpPrefix : ""}/settings`,
            {
                method:"POST",
                body: new URLSearchParams({effectInterval:interval})
            },"updateEventInterval").then(resolve)
              .then(requestRefresh)
              .catch(reject)
              .finally(()=>setRequestRunning(false));    
        });
    };

    const displayHeader = ()=>{
        return <Box className={classes.effectsHeaderValue}>
            <Typography variant="little" color="textPrimary">Interval</Typography>:
            <Link href="#" variant="little" color="textSecondary" onClick={() => setEditing(true)}>{effects.effectInterval}</Link>
        </Box>;
    };

    const editingHeader = ()=>{
        return <ClickAwayListener onClickAway={()=>{updateEventInterval(pendingInterval);setEditing(false);}}>
            <Box className={classes.effectsHeaderValue}>
            <TextField label="Interval ms"
                variant="outlined"
                type="number"
                defaultValue={effects.effectInterval}
                onChange={event => setPendingInterval(event.target.value)} />
        </Box></ClickAwayListener>;
    };

    if (!effects && open){
        return <Box>Loading....</Box>;
    }

    return effects && <Box className={`${classes.root} ${!open && classes.hidden}`}>
        <Box className={classes.effectsHeader}>
            {editing ? 
            editingHeader():
            displayHeader()}
            <Countdown 
                label="Time Remaining"
                requestRefresh={requestRefresh}
                millisecondsRemaining={effects.millisecondsRemaining}/>
            {(effects.Effects.length > 1) && <Box>
                <IconButton disabled={requestRunning} onClick={()=>navigate(false)}><Icon>skip_previous</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>navigate(true)}><Icon>skip_next</Icon></IconButton>
                <IconButton disabled={requestRunning} onClick={()=>setNextRefreshDate(Date.now())}><Icon>refresh</Icon></IconButton>
            </Box>}
        </Box>
        <Box className={classes.effects}>
            {effects.Effects.map((effect,idx) => <Effect 
                                                    key={`effect-${idx}`}
                                                    effect={effect} 
                                                    effectIndex={idx} 
                                                    navigateTo={navigateTo}
                                                    requestRunning={requestRunning}
                                                    effectEnable={effectEnable}
                                                    effectInterval={effects.effectInterval}
                                                    selected={idx === effects.currentEffect}
                                                    millisecondsRemaining={effects.millisecondsRemaining}/>)}
        </Box>
    </Box>
});