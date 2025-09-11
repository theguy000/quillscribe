#!/usr/bin/env python3
"""
QuillScribe Git Management Script
Interactive tool for pushing changes, listing tags, and deleting tags both locally and remotely.
"""

import subprocess
import sys
from typing import List, Optional
from datetime import datetime


class GitManager:
    """Git operations manager for QuillScribe project."""
    
    def __init__(self, repo_path: str = "."):
        self.repo_path = repo_path
        
    def run_git_command(self, args: List[str], capture_output: bool = True) -> subprocess.CompletedProcess:
        """Run a git command and return the result."""
        cmd = ["git"] + args
        try:
            result = subprocess.run(
                cmd, 
                cwd=self.repo_path, 
                capture_output=capture_output,
                text=True,
                check=True
            )
            return result
        except subprocess.CalledProcessError as e:
            print(f"❌ Git command failed: {' '.join(cmd)}")
            print(f"Error: {e.stderr if e.stderr else e}")
            raise
    
    def check_git_status(self) -> bool:
        """Check if we're in a git repository and get status."""
        try:
            result = self.run_git_command(["status", "--porcelain"])
            return True
        except subprocess.CalledProcessError:
            print("❌ Not in a git repository or git not available")
            return False
    
    def push_changes(self, branch: Optional[str] = None, force: bool = False) -> bool:
        """Push changes to remote repository."""
        print("🚀 Pushing changes to remote repository...")
        
        try:
            # Get current branch if not specified
            if not branch:
                result = self.run_git_command(["branch", "--show-current"])
                branch = result.stdout.strip()
                
            if not branch:
                print("❌ Could not determine current branch")
                return False
                
            print(f"📤 Pushing to branch: {branch}")
            
            # Add all changes
            self.run_git_command(["add", "."])
            
            # Check if there are changes to commit
            try:
                self.run_git_command(["diff", "--staged", "--exit-code"])
                print("ℹ️  No changes to commit")
            except subprocess.CalledProcessError:
                # There are staged changes, commit them
                timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                commit_msg = f"Auto-commit: {timestamp}"
                
                self.run_git_command(["commit", "-m", commit_msg])
                print(f"✅ Committed changes: {commit_msg}")
            
            # Push to remote
            push_args = ["push", "origin", branch]
            if force:
                push_args.append("--force")
                print("⚠️  Force pushing...")
                
            self.run_git_command(push_args)
            print(f"✅ Successfully pushed to origin/{branch}")
            return True
            
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to push changes: {e}")
            return False
    
    def get_last_tags(self, count: int = 10) -> List[str]:
        """Get the last N tags sorted by creation date."""
        print(f"🏷️  Getting last {count} tags...")
        
        try:
            # Get tags sorted by creation date (newest first)
            result = self.run_git_command([
                "tag", "--sort=-creatordate", 
                "--format=%(refname:short)|%(creatordate:short)|%(subject)"
            ])
            
            if not result.stdout.strip():
                print("ℹ️  No tags found in repository")
                return []
            
            lines = result.stdout.strip().split('\n')
            tags = []
            
            print(f"\n📋 Last {min(count, len(lines))} tags:")
            print("-" * 70)
            print(f"{'#':<3} {'Tag':<20} {'Date':<12} {'Description':<30}")
            print("-" * 70)
            
            for i, line in enumerate(lines[:count], 1):
                parts = line.split('|', 2)
                tag_name = parts[0]
                tag_date = parts[1] if len(parts) > 1 else "N/A"
                tag_desc = parts[2] if len(parts) > 2 else "N/A"
                
                tags.append(tag_name)
                print(f"{i:<3} {tag_name:<20} {tag_date:<12} {tag_desc[:30]:<30}")
            
            print("-" * 70)
            return tags
            
        except subprocess.CalledProcessError as e:
            print(f"❌ Failed to get tags: {e}")
            return []
    
    def delete_tags(self, tags: List[str], local: bool = True, remote: bool = True) -> bool:
        """Delete specified tags locally and/or remotely."""
        if not tags:
            print("ℹ️  No tags to delete")
            return True
            
        success = True
        
        # Delete locally
        if local:
            print(f"🗑️  Deleting {len(tags)} tags locally...")
            for tag in tags:
                try:
                    self.run_git_command(["tag", "-d", tag])
                    print(f"  ✅ Deleted local tag: {tag}")
                except subprocess.CalledProcessError as e:
                    print(f"  ❌ Failed to delete local tag {tag}: {e}")
                    success = False
        
        # Delete remotely
        if remote:
            print(f"🌐 Deleting {len(tags)} tags from remote...")
            for tag in tags:
                try:
                    self.run_git_command(["push", "origin", f":refs/tags/{tag}"])
                    print(f"  ✅ Deleted remote tag: {tag}")
                except subprocess.CalledProcessError as e:
                    print(f"  ❌ Failed to delete remote tag {tag}: {e}")
                    success = False
        
        return success
    
    def delete_all_last_tags(self, count: int = 10, confirm: bool = True) -> bool:
        """Delete the last N tags both locally and remotely."""
        tags = self.get_last_tags(count)
        
        if not tags:
            return True
            
        if confirm:
            print(f"\n⚠️  This will delete {len(tags)} tags both locally and remotely!")
            print("Tags to be deleted:", ", ".join(tags))
            
            response = input("\nAre you sure? (yes/no): ").lower().strip()
            if response not in ['yes', 'y']:
                print("❌ Operation cancelled")
                return False
        
        return self.delete_tags(tags, local=True, remote=True)


def get_user_input(prompt: str, default: str = "", input_type: type = str):
    """Get user input with optional default value."""
    if default:
        full_prompt = f"{prompt} [{default}]: "
    else:
        full_prompt = f"{prompt}: "
    
    try:
        user_input = input(full_prompt).strip()
        if not user_input and default:
            return default
        
        if input_type == int:
            return int(user_input) if user_input else (int(default) if default else 0)
        elif input_type == bool:
            return user_input.lower() in ['y', 'yes', 'true', '1'] if user_input else default
        else:
            return user_input
            
    except ValueError:
        print(f"❌ Invalid input. Expected {input_type.__name__}")
        return get_user_input(prompt, default, input_type)


def show_menu():
    """Display the main menu."""
    print("\n" + "="*60)
    print("🛠️  QUILLSCRIBE GIT MANAGER")
    print("="*60)
    print("1. 🚀 Push changes to remote")
    print("2. 🏷️  List recent tags")
    print("3. 🗑️  Delete tags (local and remote)")
    print("4. ❌ Exit")
    print("="*60)


def handle_push_changes(git_manager: GitManager):
    """Handle push changes workflow."""
    print("\n🚀 PUSH CHANGES TO REMOTE")
    print("-" * 40)
    
    # Get current branch
    try:
        result = git_manager.run_git_command(["branch", "--show-current"])
        current_branch = result.stdout.strip()
        print(f"Current branch: {current_branch}")
    except:
        current_branch = "main"
    
    # Get user options
    branch = get_user_input("Branch to push to", current_branch)
    force = get_user_input("Force push? (y/n)", "n", bool)
    
    if force:
        confirm = get_user_input("⚠️  Force push can overwrite remote history. Continue? (y/n)", "n", bool)
        if not confirm:
            print("❌ Operation cancelled")
            return
    
    # Execute push
    success = git_manager.push_changes(branch=branch, force=force)
    if success:
        print("✅ Push completed successfully!")
    else:
        print("❌ Push failed!")


def handle_list_tags(git_manager: GitManager):
    """Handle list tags workflow."""
    print("\n🏷️  LIST RECENT TAGS")
    print("-" * 40)
    
    count = get_user_input("Number of tags to show", "10", int)
    git_manager.get_last_tags(count)


def handle_delete_tags(git_manager: GitManager):
    """Handle delete tags workflow."""
    print("\n🗑️  DELETE TAGS")
    print("-" * 40)
    
    # First show current tags
    count = get_user_input("Number of recent tags to show/delete", "10", int)
    tags = git_manager.get_last_tags(count)
    
    if not tags:
        print("ℹ️  No tags to delete")
        return
    
    print("\nDeletion options:")
    print("1. Delete both locally and remotely (recommended)")
    print("2. Delete only locally")
    print("3. Delete only remotely")
    print("4. Cancel")
    
    choice = get_user_input("Choose option (1-4)", "1", int)
    
    if choice == 4:
        print("❌ Operation cancelled")
        return
    
    local = choice in [1, 2]
    remote = choice in [1, 3]
    
    # Show what will be deleted
    print(f"\n⚠️  This will delete {len(tags)} tags:")
    for i, tag in enumerate(tags, 1):
        print(f"  {i}. {tag}")
    
    location_text = []
    if local:
        location_text.append("locally")
    if remote:
        location_text.append("remotely")
    
    print(f"\nTags will be deleted: {' and '.join(location_text)}")
    
    confirm = get_user_input("Are you absolutely sure? Type 'DELETE' to confirm", "")
    if confirm != "DELETE":
        print("❌ Operation cancelled")
        return
    
    # Execute deletion
    success = git_manager.delete_tags(tags, local=local, remote=remote)
    if success:
        print("✅ Tags deleted successfully!")
    else:
        print("❌ Some tag deletions failed!")


def main():
    """Main interactive entry point."""
    print("🛠️  QuillScribe Git Management Tool")
    print("Interactive mode - no command line arguments needed!")
    
    # Initialize Git manager
    git_manager = GitManager()
    
    # Check if we're in a git repository
    if not git_manager.check_git_status():
        return 1
    
    try:
        while True:
            show_menu()
            
            choice = get_user_input("Select an option (1-4)", "4", int)
            
            if choice == 1:
                handle_push_changes(git_manager)
            elif choice == 2:
                handle_list_tags(git_manager)
            elif choice == 3:
                handle_delete_tags(git_manager)
            elif choice == 4:
                print("\n👋 Goodbye!")
                break
            else:
                print("❌ Invalid choice. Please select 1-4.")
            
            # Ask if user wants to continue
            if choice in [1, 2, 3]:
                continue_choice = get_user_input("\nPress Enter to continue or 'q' to quit", "")
                if continue_choice.lower() == 'q':
                    print("\n👋 Goodbye!")
                    break
                    
    except KeyboardInterrupt:
        print("\n\n👋 Goodbye!")
        return 0
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        return 1
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
