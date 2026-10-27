#include "github/overview.hpp"
#include "github/gh_client.hpp"
#include "github/service.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <algorithm>

namespace kepp::github {
using namespace ftxui;

int run_overview() {
  RepositoryRegistry registry;
  auto repos = registry.load();
  GhClient client;
  int repo_i = 0, item_i = 0, tab = 0, active_input = 0, filter_i = 0;
  int limit = 30;
  std::string status = "Press a to register OWNER/REPO", query, input;
  bool editing = false, searching = false, detail = false;
  std::vector<Issue> issues;
  std::vector<PullRequest> prs;
  std::vector<WorkflowRun> runs;
  const std::vector<std::vector<std::string>> filters = {
      {"all", "open", "closed", "agent:ready"},
      {"all", "open", "closed", "merged", "draft"},
      {"all", "failed", "running"}};

  auto refresh = [&] {
    if (repos.empty()) { status = "No repositories registered"; return; }
    try {
      if (tab == 0) issues = client.issues(repos[repo_i], "all", limit);
      else if (tab == 1) prs = client.pull_requests(repos[repo_i], limit);
      else runs = client.runs(repos[repo_i], limit);
      status = "Updated " + repos[repo_i]; item_i = 0;
    } catch (const std::exception &e) { status = e.what(); }
  };

  auto visible_issues = [&] {
    auto f = filters[0][filter_i];
    auto out = filter_issues(issues, query, f == "agent:ready" ? f : "");
    if (f == "open" || f == "closed")
      out.erase(std::remove_if(out.begin(), out.end(), [&](const Issue &i) {
        std::string state = i.state; std::transform(state.begin(), state.end(), state.begin(), ::tolower);
        return state != f;
      }), out.end());
    return out;
  };
  auto visible_prs = [&] { return filter_prs(prs, filters[1][filter_i] == "draft" ? "all" : filters[1][filter_i], filters[1][filter_i] == "draft"); };
  auto visible_runs = [&] { return filter_runs(runs, filters[2][filter_i]); };

  auto screen = ScreenInteractive::Fullscreen();
  auto repo_input = Input(&input, "owner/repository");
  auto search_input = Input(&query, "title or number");
  auto base = Container::Tab({repo_input, search_input}, &active_input);
  auto renderer = Renderer(base, [&] {
    if (editing) return vbox({text("Register repository") | bold, repo_input->Render() | border, text("Enter: Save  Esc: Cancel")}) | center;
    if (searching) return vbox({text("Search current list") | bold, search_input->Render() | border, text("Enter: Apply  Esc: Clear")}) | center;
    Elements left;
    for (int i = 0; i < (int)repos.size(); ++i) left.push_back(text((i == repo_i ? "> " : "  ") + repos[i]) | (i == repo_i ? bold : nothing));
    Elements rows;
    if (tab == 0) { auto v = visible_issues(); for (int i=0;i<(int)v.size();++i) rows.push_back(text((i==item_i?"> ":"  ")+std::string("#")+std::to_string(v[i].number)+" "+v[i].title)); }
    else if (tab == 1) { auto v=visible_prs(); for(int i=0;i<(int)v.size();++i) rows.push_back(text((i==item_i?"> ":"  ")+std::string("#")+std::to_string(v[i].number)+(v[i].is_draft?" [Draft] ":" ")+v[i].title)); }
    else { auto v=visible_runs(); for(int i=0;i<(int)v.size();++i) rows.push_back(text((i==item_i?"> ":"  ")+v[i].workflow_name+" ["+(v[i].status=="completed"?v[i].conclusion:v[i].status)+"]")); }
    Element body = hbox({vbox({text(" REPOSITORIES ")|bold,separator(),vbox(left)|flex})|size(WIDTH,EQUAL,28)|border,
                         vbox({text(tab==0?" Issues ":tab==1?" Pull Requests ":" Actions ")|bold,separator(),vbox(rows)|frame|flex})|flex|border});
    if (detail) body=vbox({text(" Detail ")|bold,separator(),paragraph(status),separator(),text("Esc: Back")})|border;
    return vbox({text("KEPP / GitHub Console")|bold|color(Color::Cyan),text(repos.empty()?"":repos[repo_i]),
                 text("Filter: "+filters[tab][filter_i]+(query.empty()?"":" · Search: "+query)+" · Limit: "+std::to_string(limit))|dim,
                 body|flex,separator(),paragraph(status),
                 text("j/k Select · h/l Repository · Tab Views · Enter Details · / Search · f Filter · m More · a Add · d Unregister · r Refresh · q Back")|dim});
  });

  auto component = CatchEvent(renderer, [&](Event e) {
    if (editing) {
      if (e==Event::Escape){editing=false;input.clear();return true;}
      if (e==Event::Return){if(!valid_repository_name(input))status="Use OWNER/REPO format";else if(std::find(repos.begin(),repos.end(),input)==repos.end()){repos.push_back(input);std::sort(repos.begin(),repos.end());registry.save(repos);repo_i=std::find(repos.begin(),repos.end(),input)-repos.begin();editing=false;input.clear();refresh();}return true;}
      return false;
    }
    if (searching) { if(e==Event::Return){searching=false;item_i=0;return true;}if(e==Event::Escape){searching=false;query.clear();item_i=0;return true;}return false; }
    if (detail) { if(e==Event::Escape||e==Event::Return){detail=false;return true;}return false; }
    if(e==Event::Character('q')){screen.Exit();return true;}
    if(e==Event::Tab){tab=(tab+1)%3;filter_i=0;query.clear();refresh();return true;}
    if(e==Event::Character('/')){searching=true;active_input=1;query.clear();return true;}
    if(e==Event::Character('f')){filter_i=(filter_i+1)%filters[tab].size();item_i=0;return true;}
    if(e==Event::Character('m')){limit+=30;refresh();return true;}
    if(e==Event::Character('a')){editing=true;active_input=0;return true;}
    if(e==Event::Character('d')&&!repos.empty()){repos.erase(repos.begin()+repo_i);repo_i=std::max(0,std::min(repo_i,(int)repos.size()-1));registry.save(repos);issues.clear();prs.clear();runs.clear();status="Repository unregistered";return true;}
    if(e==Event::Character('r')){refresh();return true;}
    if((e==Event::Character('l')||e==Event::ArrowRight)&&repo_i+1<(int)repos.size()){++repo_i;refresh();return true;}
    if((e==Event::Character('h')||e==Event::ArrowLeft)&&repo_i>0){--repo_i;refresh();return true;}
    int count=tab==0?visible_issues().size():tab==1?visible_prs().size():visible_runs().size();
    if((e==Event::Character('j')||e==Event::ArrowDown)&&item_i+1<count){++item_i;return true;}
    if((e==Event::Character('k')||e==Event::ArrowUp)&&item_i>0){--item_i;return true;}
    if(e==Event::Return&&count){try{if(tab==0){auto v=visible_issues();auto x=client.issue(repos[repo_i],v[item_i].number);status="#"+std::to_string(x.number)+" "+x.title+"\n\n"+x.body;for(auto&c:x.comments)status+="\n\n"+c.author+": "+c.body;}else if(tab==1){auto v=visible_prs();auto x=client.pull_request(repos[repo_i],v[item_i].number);status="#"+std::to_string(x.number)+" "+x.title+"\n"+x.head_branch+" · "+x.review_decision+"\n\n"+x.body;for(auto&f:x.files)status+="\n  "+f;for(auto&c:x.checks)status+="\n  ["+(c.conclusion.empty()?c.status:c.conclusion)+"] "+c.name;}else{auto v=visible_runs();auto x=client.run(repos[repo_i],v[item_i].id);status=x.workflow_name+" · "+x.status+" / "+x.conclusion+"\n\n"+x.failed_log;}detail=true;}catch(const std::exception&x){status=x.what();}return true;}
    return false;
  });
  if(!repos.empty())refresh();else{std::string error;if(!client.authenticated(error))status="gh authentication unavailable: "+error;}
  screen.Loop(component);return 0;
}
} // namespace kepp::github
