(function(){
  var COL={1:'var(--r)',2:'var(--g)',4:'var(--b)',3:'var(--y)',5:'var(--m)',6:'var(--c)',7:'var(--w)'};
  var NAME={1:'Red',2:'Green',4:'Blue',3:'Yellow',5:'Magenta',6:'Cyan',7:'White',0:'—'};
  var TARGETS=[1,2,4,3,5,6,7];
  function paint(el,v){ if(v){el.style.background=COL[v];el.style.boxShadow='0 0 24px '+COL[v];el.classList.add('on');} else {el.style.background='';el.style.boxShadow='';el.classList.remove('on');} }
  function caps(id,v){ var c=document.getElementById(id).children; [1,2,4].forEach(function(bit,i){ c[i].classList.toggle('down',!!(v&bit)); }); }
  function rnd(a){return a[Math.floor(Math.random()*a.length)];}

  /* ---- hero demo loop ---- */
  var lcd=document.getElementById('lcd'),T=document.getElementById('ledT'),L1=document.getElementById('led1'),L2=document.getElementById('led2');
  var sc=[0,0], reduce=window.matchMedia&&matchMedia('(prefers-reduced-motion: reduce)').matches;
  function lcdSet(a,b){ lcd.textContent=(a||'').padEnd(16,' ').slice(0,16)+'\n'+(b||'').padEnd(16,' ').slice(0,16); }
  function scoreLine(){ return 'P1: '+sc[0]+' vs P2: '+sc[1]; }
  function dots(){ [0,1].forEach(function(p){ var d=document.getElementById('s'+(p+1)).children; for(var i=0;i<3;i++) d[i].classList.toggle('on',i<sc[p]); }); }
  function setP(p,v){ paint(p===1?L1:L2,v); caps('b'+p,v); }
  /* State 4: tug-of-war scoring */
  function award(w){ var o=w===1?1:0; if(sc[o]>0) sc[o]--; else sc[w-1]++; }
  var q=[]; function at(ms,fn){q.push([ms,fn]);}
  function run(){ var t=0; q.forEach(function(s){ t+=s[0]; setTimeout(s[1],t); }); q=[]; setTimeout(build,t+400); }
  function mixRound(win){
    var lose=win===1?2:1, tgt=rnd(TARGETS), bits=[1,2,4].filter(function(b){return tgt&b;});
    var wrong=rnd(TARGETS.filter(function(v){return v!==tgt;}));
    at(500,function(){ paint(T,tgt); setP(1,0); setP(2,0); lcdSet(scoreLine(),'   MIX COLOR!'); });
    var acc=0; bits.forEach(function(b){ acc|=b; var v=acc; at(450,function(){ setP(win,v); }); });
    at(-200,function(){ setP(lose,wrong); });
  }
  function memRound(win){
    var n=2+Math.floor(Math.random()*3), seq=[]; for(var i=0;i<n;i++) seq.push(rnd(TARGETS));
    at(500,function(){ setP(1,0); setP(2,0); lcdSet('  MEMORY TIME!',' Watch '+n+' colors'); });
    seq.forEach(function(c){ at(550,function(){ paint(T,c); }); at(450,function(){ paint(T,0); }); });
    at(300,function(){ lcdSet(scoreLine(),'  Your turn!'); });
    seq.forEach(function(c){ at(380,function(){ setP(win,c); }); at(160,function(){ setP(win,0); }); });
  }
  function build(){
    sc=[0,0]; dots(); paint(T,0); setP(1,0); setP(2,0);
    at(0,function(){ lcdSet(' COLOR MIX GAME','Press All 6 Btns'); });
    at(1800,function(){ lcdSet('     Ready','      3..'); });
    at(700,function(){ lcdSet('     Ready','      2..'); });
    at(700,function(){ lcdSet('     Ready','      1..'); });
    at(700,function(){ lcdSet('','      GO!'); });
    var s=[0,0], guard=0, fav=Math.random()<.5?1:2;
    while(s[0]<3&&s[1]<3&&guard++<30){
      var w=Math.random()<.72?fav:(fav===1?2:1), o=w===1?1:0;
      if(Math.random()<.65) mixRound(w); else memRound(w);
      if(s[o]>0) s[o]--; else s[w-1]++;
      (function(win){ at(600,function(){ award(win); dots(); lcdSet(scoreLine(),'P'+win+' SCORE!  +1'); }); })(w);
      at(1000,function(){});
    }
    var champ=s[0]>=s[1]?1:2;
    at(200,function(){ lcdSet('  PLAYER '+champ+' WIN!!','Press 6B: Reset'); paint(T,6); setP(1,0); setP(2,0); });
    for(var i=0;i<6;i++){ (function(k){ at(240,function(){ var d=document.getElementById('s'+champ).children; for(var j=0;j<3;j++) d[j].classList.toggle('on',k%2===1); }); })(i); }
    at(1800,function(){});
    run();
  }
  if(reduce){ sc=[1,0]; dots(); paint(T,3); setP(1,3); setP(2,1); lcdSet(scoreLine(),'   MIX COLOR!'); }
  else build();

  /* ---- mixer ---- */
  var mxT=document.getElementById('mxT'),mxP=document.getElementById('mxP'),ver=document.getElementById('verdict');
  var btn={1:document.getElementById('mxR'),2:document.getElementById('mxG'),4:document.getElementById('mxB')};
  var cur=0,tg=0;
  function upd(){ paint(mxP,cur); [1,2,4].forEach(function(b){btn[b].setAttribute('aria-pressed',(cur&b)?'true':'false');});
    if(!cur){ver.textContent='';ver.style.color='';return;}
    if(cur===tg){ver.textContent='ถูกต้อง! '+NAME[cur];ver.style.color='var(--g)';}
    else {ver.textContent=NAME[cur]+' ยังไม่ตรงโจทย์';ver.style.color='var(--muted)';} }
  function newT(){ var n; do{n=rnd(TARGETS);}while(n===tg); tg=n; cur=0; paint(mxT,tg); upd(); }
  [1,2,4].forEach(function(b){ btn[b].addEventListener('click',function(){cur^=b;upd();}); });
  document.getElementById('mxNew').addEventListener('click',newT);
  document.addEventListener('keydown',function(e){ if(e.target.tagName==='INPUT')return; var k={r:1,g:2,b:4}[e.key.toLowerCase()]; if(k){cur^=k;upd();} });
  document.querySelectorAll('[data-placeholder]').forEach(function(a){ a.addEventListener('click',function(e){e.preventDefault();}); });
  newT();
})();
